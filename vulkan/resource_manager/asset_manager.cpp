#include "asset_manager.h"

#include <algorithm>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <tuple>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/util/index_buffer_util.h"

std::unique_ptr<AssetManager> AssetManager::create(
    const LogicalDevice& logicalDevice, BufferManager& bufferManager) {
  return std::unique_ptr<AssetManager>(new AssetManager(logicalDevice, bufferManager, 19));
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

  std::expected<Ref<VirtualAllocation>, VirtualAllocation> expectedRef =
      std::unexpected(std::move(virtualAllocation));
  for (uint8_t i = 0; i < threadData.virtualAllocationCounters.size(); i++) {
    // Fast path: virtual allocation counters have a free spot.
    expectedRef = threadData.virtualAllocationCounters[i]->transferResource(
        std::move(expectedRef.error()), virtualAllocationMetadata);
    if (expectedRef.has_value()) {
      break;
    }
  }

  if (!expectedRef.has_value()) [[unlikely]] {
    // Slow path: very rare, if no virtual allocation counter has free spot then allocate the
    // new one.
    threadData.virtualAllocationCounters.push_back(
        std::make_unique<ReferenceCounterWithMetadata<VirtualAllocation>>());
    expectedRef = threadData.virtualAllocationCounters.back()->transferResource(
        std::move(expectedRef.error()), virtualAllocationMetadata);
    if (!expectedRef.has_value()) [[unlikely]] {
      throw EngineException("Failed to store the virtual allocation.");
    }
  }
  return std::make_tuple(threadData.bufferBlocks.back().stagingBuffer, std::move(*expectedRef),
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
      common::copyData(
          std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                        + virtualAllocationMetadata.offset,
                    virtualAllocationMetadata.size),
          0, std::span(static_cast<const std::byte*>(resource.data), resource.size));

      promise->stagingBuffer = std::move(stagingBufferRef);
      promise->virtualAllocation = std::move(virtualAllocationRef);
      promise->width = resource.width;
      promise->height = resource.height;
      promise->mipLevels = resource.mipLevels;
      promise->layerCount = resource.layerCount;
      promise->copyRegions = std::move(resource.subresources);
      promise->residentMips.store(0, std::memory_order_relaxed);
      promise->loadState.store(common::AssetManager::LoadState::READY, std::memory_order_release);
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
    _tasks.push_back([this, promise, modelPtr = std::move(modelPtr),
                      resource = std::move(resource)](
                         ThreadData& threadData, size_t blockSize, size_t alignment) {
      auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] =
          allocate(threadData, resource.size, alignment, blockSize);

      common::copyData(
          std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                        + virtualAllocationMetadata.offset,
                    virtualAllocationMetadata.size),
          0, std::span(static_cast<const std::byte*>(resource.data), resource.size));

      promise->stagingBuffer = std::move(stagingBufferRef);
      promise->virtualAllocation = std::move(virtualAllocationRef);
      promise->width = resource.width;
      promise->height = resource.height;
      promise->mipLevels = resource.mipLevels;
      promise->layerCount = resource.layerCount;
      promise->copyRegions = std::move(resource.subresources);
      promise->residentMips.store(0, std::memory_order_relaxed);
      promise->loadState.store(common::AssetManager::LoadState::READY, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}

std::shared_ptr<common::AssetManager::VertexData> AssetManager::loadVertexDataInterleavingAsync(
    std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, IndexType indexType,
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

      const IndexType shrunkIndexType = getShrunkIndexSize(indices, indexType);
      auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] = allocate(
          threadData,
          indices.size() / static_cast<size_t>(indexType) * static_cast<size_t>(shrunkIndexType),
          alignment, blockSize);
      common::copyAndShrinkIndexData(
          std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory
                        + virtualAllocationMetadata.offset,
                    virtualAllocationMetadata.size),
          indices, static_cast<size_t>(shrunkIndexType), static_cast<size_t>(indexType));

      promise->indexType = shrunkIndexType;
      promise->indexBuffer =
          std::make_tuple(std::move(stagingBufferRef), std::move(virtualAllocationRef));
      promise->loadState.store(common::AssetManager::LoadState::READY, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}
