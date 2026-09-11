#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <future>
#include <iostream>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "common/buffer/buffer.h"
#include "common/model_loader/image_loader/types.h"
#include "common/util/asset_manager.h"
#include "common/util/buffer_manip.h"
#include "common/util/ref.h"
#include "common/util/resource_handles.h"
#include "lib/association_list/association_list.h"
#include "lib/buffer/buffer.h"
#include "lib/sparse/sparse_map.h"
#include "lib/types/memory.h"
#include "vulkan/resource_manager/buffer_manager.h"
#include "vulkan/resource_manager/reference_counter_with_metadata.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/util/check.h"

class AssetManager : public common::AssetManager {
  AssetManager(const LogicalDevice& logicalDevice, std::launch launchPolicy);

public:
  static std::unique_ptr<AssetManager> create(
      const LogicalDevice& logicalDevice, std::launch launchPolicy = std::launch::async);

  ~AssetManager() = default;

  struct ImageData {
    std::tuple<Buffer, BufferMetadata> stagingBuffer;
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;
    lib::Buffer<VkBufferImageCopy> copyRegions;
  };

  struct VertexData {
    lib::DynamicAssociationList<std::string, std::tuple<Buffer, BufferMetadata>> buffers;
    std::tuple<Buffer, BufferMetadata> indexBuffer;
    VkIndexType indexType;
  };

  StagingImageDataResourceHandle loadImageAsync(
      std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) override;

  StagingImageDataResourceHandle loadImageAsync(
      std::shared_ptr<void> modelPtr, ImageResource&& imageResource) override;

  StagingVertexDataResourceHandle loadVertexDataInterleavingAsync(
      std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, uint8_t indexSize,
      std::vector<common::BufferDescription>&& bufferDescriptions) override;

  const ImageData& getImageData(StagingImageDataResourceHandle index);

  ImageData releaseImageData(StagingImageDataResourceHandle index);

  const VertexData& getVertexData(StagingVertexDataResourceHandle index);

  VertexData releaseVertexData(StagingVertexDataResourceHandle index);

private:
  using ImageResourceMap = lib::SparseMap<ImageData, MAX_STAGING_IMAGE_DATA_RESOURCES>;
  using VertexResourceMap = lib::SparseMap<VertexData, MAX_STAGING_VERTEX_DATA_RESOURCES>;

  std::launch _launchPolicy;

  const LogicalDevice& _logicalDevice;

  std::vector<StagingImageDataResourceHandle> _freeImageDataIndices;
  std::unordered_map<StagingImageDataResourceHandle, std::future<ImageData>>
      _awaitingImageDataResources;  // TODO: Change to flat unordered map.
  ImageResourceMap _imageDataResources;

  std::vector<StagingVertexDataResourceHandle> _freeVertexDataIndices;
  std::unordered_map<StagingVertexDataResourceHandle, std::future<VertexData>>
      _awaitingVertexDataResources;  // TODO: Change to flat unordered map.
  VertexResourceMap _vertexDataResources;
};

class NewAssetManager {
  struct ThreadData {
    std::thread thread;
    struct BufferBlock {
      Ref<Buffer> stagingBuffer;
      VirtualBlock virtualBlock;
    };
    std::deque<BufferBlock> bufferBlocks;  // We want pointer stability.
    std::optional<BufferBlock> blockToBeReclaimed;
    std::vector<std::unique_ptr<ReferenceCounterWithMetadata<VirtualAllocation>>>
        virtualAllocationCounters;
    std::optional<std::unique_ptr<ReferenceCounterWithMetadata<VirtualAllocation>>>
        virtualAllocationCouterToBeReclaimed;
  };

  NewAssetManager(const LogicalDevice& logicalDevice, BufferManager& bufferManager,
                  uint8_t threadCount, size_t size = 2 * lib::GiB)
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
      _threads[i].virtualAllocationCounters.push_back(
          std::make_unique<ReferenceCounterWithMetadata<VirtualAllocation>>());
      _threads[i].thread = std::thread(&NewAssetManager::doWork, this, i);
    }
    _tasks.reserve(256);
  }

  void doWork(uint8_t threadIndex) {
    ThreadData& thisThread = _threads[threadIndex];
    std::function<void(ThreadData&, size_t, size_t)> task;
    bool timedOut;
    while (true) {
      {
        std::unique_lock lock(_mutex);
        timedOut = !_conditionVariable.wait_for(lock, std::chrono::seconds(10000), [this] {
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
        cleanVirtualAllocatorCounters(thisThread);
        cleanVirtualBlocks(thisThread);
      }
    }
  }

  // Pop from the back of the vector as long as the size of the allocation is 0.
  void cleanVirtualAllocatorCounters(ThreadData& threadData) {
    while (threadData.virtualAllocationCounters.size() > 1
           && threadData.virtualAllocationCounters.back()->size() == 0) {
      if (!threadData.virtualAllocationCouterToBeReclaimed.has_value()) {
        threadData.virtualAllocationCouterToBeReclaimed =
            std::move(threadData.virtualAllocationCounters.back());
      }
      threadData.virtualAllocationCounters.pop_back();
    }
  }

  void cleanVirtualBlocks(ThreadData& threadData) {
    while (threadData.bufferBlocks.size() > 1
           && threadData.bufferBlocks.front().virtualBlock.empty()) {
      if (!threadData.blockToBeReclaimed.has_value()) {
        threadData.blockToBeReclaimed.emplace(std::move(threadData.bufferBlocks.front()));
      }
      threadData.bufferBlocks.pop_front();
    }
  }

public:
  static std::unique_ptr<NewAssetManager> create(
      const LogicalDevice& logicalDevice, BufferManager& bufferManager);

  ~NewAssetManager();

  enum class LoadState : uint8_t {
    PENDING,
    PARTIAL,
    READY
  };

  struct ImageData {
    Ref<Buffer> stagingBuffer;
    Ref<VirtualAllocation> virtualAllocation;
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;
    size_t bufferOffset;
    lib::Buffer<ImageSubresource> copyRegions;
    std::atomic<uint8_t> residentMips = 0;
    std::atomic<LoadState> loadState = LoadState::PENDING;
  };

  struct VertexData {
    lib::DynamicAssociationList<std::string, std::tuple<common::Ref, common::Ref>> buffers;
    std::tuple<common::Ref, common::Ref> indexBuffer;
    IndexType indexType;
    std::atomic<LoadState> loadState = LoadState::PENDING;
  };

  // TODO virtual override
  std::shared_ptr<NewAssetManager::ImageData> loadImageAsync(
      std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction);

  // TODO virtual override
  std::shared_ptr<NewAssetManager::ImageData> loadImageAsync(
      std::shared_ptr<void> modelPtr, ImageResource&& imageResource);

  // TODO virtual override
  std::shared_ptr<NewAssetManager::VertexData> loadVertexDataInterleavingAsync(
      std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, IndexType indexType,
      std::vector<common::BufferDescription>&& bufferDescriptions);

private:
  std::tuple<Ref<Buffer>, Ref<VirtualAllocation>, VirtualAllocationMetadata> allocate(
      ThreadData& threadData, size_t size, size_t alignment, size_t blockSize);

  const LogicalDevice& _logicalDevice;
  BufferManager& _bufferManager;
  lib::Buffer<ThreadData> _threads;
  const size_t _bufferSize;
  const size_t _alignment;

  std::mutex _mutex;
  std::condition_variable _conditionVariable;
  std::vector<std::function<void(ThreadData&, size_t, size_t)>> _tasks;
  bool _stop = false;
};
