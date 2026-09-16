#include "asset_manager.h"

#include <algorithm>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <tuple>
#include <vulkan/vulkan.h>

#include "common/buffer/index_buffer_lib.h"
#include "common/buffer/vertex_buffer_lib.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"

std::unique_ptr<AssetManager> AssetManager::create(
    const LogicalDevice& logicalDevice, BufferManager& bufferManager) {
  return std::unique_ptr<AssetManager>(new AssetManager(
      logicalDevice, bufferManager, std::thread::hardware_concurrency() - 1, 2 * lib::GiB));
}

AssetManager::AssetManager(const LogicalDevice& logicalDevice, BufferManager& bufferManager,
                           uint8_t threadCount, size_t size)
  : _logicalDevice(logicalDevice), _bufferManager(bufferManager), _threads(threadCount),
    _bufferSize(size / threadCount),
    _alignment(logicalDevice.getPhysicalDevice().getStagingAlignment()) {
  for (uint8_t i = 0; i < _threads.size(); i++) {
    auto [buffer, metadata] =
        BufferBuilder()
            .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .withSize(_bufferSize)
            .buildStagingBufferWithMetadata(_logicalDevice);
    _threads[i].bufferBlocks.push_back(ThreadData::BufferBlock{
      .stagingBuffer = bufferManager.storeBuffer(std::move(buffer), metadata),
      .virtualBlock = VirtualBlock::create(logicalDevice.getMemoryAllocator(), _bufferSize)});
    _threads[i].thread = std::thread(&AssetManager::doWork, this, i);
  }
  _tasks.reserve(256);
}

void AssetManager::doWork(uint8_t threadIndex) {
  ThreadData& thisThread = _threads[threadIndex];
  std::function<void(ThreadData&, size_t, size_t)> task;
  bool timedOut;
  while (true) {
    {
      std::unique_lock lock(_mutex);
      timedOut = !_conditionVariable.wait_for(lock, std::chrono::seconds(5), [this] {
        return !_tasks.empty() || _stop;
      });

      if (_stop) [[unlikely]] {
        return;
      }

      if (!timedOut) {
        task = std::move(_tasks.back());
        _tasks.pop_back();
      }
    }

    if (!timedOut) {
      task(thisThread, _bufferSize, _alignment);
    } else {
      thisThread.virtualAllocationStrategy.cleanEmptyCounters();
      cleanVirtualBlocks(thisThread);
    }
  }
}

void AssetManager::cleanVirtualBlocks(ThreadData& threadData) {
  while (
      threadData.bufferBlocks.size() > 1 && threadData.bufferBlocks.front().virtualBlock.empty()) {
    if (!threadData.blockToBeReclaimed.has_value()) {
      threadData.blockToBeReclaimed.emplace(std::move(threadData.bufferBlocks.front()));
    }
    threadData.bufferBlocks.pop_front();
  }
}

AssetManager::~AssetManager() {
  {
    std::lock_guard lck(_mutex);
    _stop = true;
  }
  _conditionVariable.notify_all();
  for (auto& thread : _threads) {
    thread.thread.join();
  }
}

std::tuple<Ref<Buffer>, Ref<VirtualAllocation>, VirtualAllocationMetadata> AssetManager::allocate(
    ThreadData& threadData, size_t size, size_t alignment, size_t blockSize) {
  std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, VirtualAllocation::Error>
      expectedVirtualAllocation =
          threadData.bufferBlocks.back().virtualBlock.createVirtualAllocation(size, alignment);
  if (!expectedVirtualAllocation.has_value()) {
    // Retry with the new buffer/block.
    if (threadData.blockToBeReclaimed.has_value()) {
      // Slower path: still very fast, if allocation didn't succeed then try to reuse the
      // retired block.
      threadData.bufferBlocks.push_back(std::move(*threadData.blockToBeReclaimed));
      threadData.blockToBeReclaimed = std::nullopt;
    } else {
      // The slowest path: allocate new staging buffer and virtual block for the allocation.
      auto [buffer, metadata] =
          BufferBuilder()
              .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
              .withSize(std::max(blockSize, size))  // Rare situation when size is bigger than
                                                    // blockSize.
              .buildStagingBufferWithMetadata(_logicalDevice);
      threadData.bufferBlocks.push_back(ThreadData::BufferBlock{
        .stagingBuffer = _bufferManager.storeBuffer(std::move(buffer), metadata),
        .virtualBlock = VirtualBlock::create(_logicalDevice.getMemoryAllocator(), blockSize)});
    }
    expectedVirtualAllocation =
        threadData.bufferBlocks.back().virtualBlock.createVirtualAllocation(size, alignment);
    if (!expectedVirtualAllocation.has_value()) [[unlikely]] {
      throw EngineException("Failed to create virtual allocation.");
    }
  }
  auto& [virtualAllocation, virtualAllocationMetadata] = expectedVirtualAllocation.value();
  return std::make_tuple(threadData.bufferBlocks.back().stagingBuffer,
                         threadData.virtualAllocationStrategy.transferResource(
                             std::move(virtualAllocation), virtualAllocationMetadata),
                         virtualAllocationMetadata);
}

std::shared_ptr<common::AssetManager::ImageData> AssetManager::loadImageAsync(
    std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) {
  auto promise = std::make_shared<common::AssetManager::ImageData>();
  {
    std::lock_guard lock(_mutex);
    _tasks.push_back([this, promise, imageFunction = std::move(imageFunction)](
                         ThreadData& threadData, size_t blockSize, size_t alignment) {
      auto [resource, dataPtr] = imageFunction();
      auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] =
          allocate(threadData, resource.size, alignment, blockSize);
      std::memcpy(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                      + virtualAllocationMetadata.offset,
                  resource.data, resource.size);

      promise->stagingBuffer = std::move(stagingBufferRef);
      promise->virtualAllocation = std::move(virtualAllocationRef);
      promise->width = resource.width;
      promise->height = resource.height;
      promise->mipLevels = resource.mipLevels;
      promise->layerCount = resource.layerCount;
      promise->copyRegions = std::move(resource.subresources);
      promise->residentMips.store(0, std::memory_order_relaxed);
      promise->loadState.store(
          common::AssetManager::LoadState::FINISHED, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}

std::shared_ptr<common::AssetManager::ImageData> AssetManager::loadImageAsync(
    std::shared_ptr<void> modelPtr, ImageResource&& resource) {
  auto promise = std::make_shared<common::AssetManager::ImageData>();
  {
    std::lock_guard lock(_mutex);
    _tasks.push_back(
        [this, promise, modelPtr = std::move(modelPtr), resource = std::move(resource)](
            ThreadData& threadData, size_t blockSize, size_t alignment) {
          auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] =
              allocate(threadData, resource.size, alignment, blockSize);
          std::memcpy(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                          + virtualAllocationMetadata.offset,
                      resource.data, resource.size);

          promise->stagingBuffer = std::move(stagingBufferRef);
          promise->virtualAllocation = std::move(virtualAllocationRef);
          promise->width = resource.width;
          promise->height = resource.height;
          promise->mipLevels = resource.mipLevels;
          promise->layerCount = resource.layerCount;
          promise->copyRegions = std::move(resource.subresources);
          promise->residentMips.store(0, std::memory_order_relaxed);
          promise->loadState.store(
              common::AssetManager::LoadState::FINISHED, std::memory_order_release);
        });
  }
  _conditionVariable.notify_one();
  return promise;
}

std::shared_ptr<common::AssetManager::VertexData> AssetManager::loadVertexDataInterleavingAsync(
    std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, common::IndexType indexType,
    std::vector<common::BufferDescription>&& bufferDescriptions) {
  auto promise = std::make_shared<common::AssetManager::VertexData>();
  {
    std::lock_guard lock(_mutex);
    _tasks.push_back([this, promise, modelPtr = std::move(modelPtr), indices, indexType,
                      bufferDescriptions = std::move(bufferDescriptions)](
                         ThreadData& threadData, size_t blockSize, size_t alignment) mutable {
      for (common::BufferDescription& description : bufferDescriptions) {
        auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] =
            allocate(threadData, description.totalSize, alignment, blockSize);
        common::copyDataInterleaving(
            std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                          + virtualAllocationMetadata.offset,
                      virtualAllocationMetadata.size),
            description.attributes);
        promise->buffers.insert(
            {std::move(description.name),
             std::make_tuple(std::move(stagingBufferRef), std::move(virtualAllocationRef))});
      }

      const common::IndexType shrunkIndexType = common::getShrunkIndexSize(indices, indexType);
      auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] = allocate(
          threadData,
          indices.size() / static_cast<size_t>(indexType) * static_cast<size_t>(shrunkIndexType),
          alignment, blockSize);
      common::shrinkIndexData(
          std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                        + virtualAllocationMetadata.offset,
                    virtualAllocationMetadata.size),
          indices, shrunkIndexType, indexType);

      promise->indexType = shrunkIndexType;
      promise->indexBuffer =
          std::make_tuple(std::move(stagingBufferRef), std::move(virtualAllocationRef));
      promise->loadState.store(
          common::AssetManager::LoadState::FINISHED, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}
