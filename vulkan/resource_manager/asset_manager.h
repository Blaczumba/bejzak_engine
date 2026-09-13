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

  AssetManager(const LogicalDevice& logicalDevice, BufferManager& bufferManager,
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
      _threads[i].thread = std::thread(&AssetManager::doWork, this, i);
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
  static std::unique_ptr<AssetManager> create(
      const LogicalDevice& logicalDevice, BufferManager& bufferManager);

  ~AssetManager();

  std::shared_ptr<common::AssetManager::ImageData> loadImageAsync(
      std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) override;

  std::shared_ptr<common::AssetManager::ImageData> loadImageAsync(
      std::shared_ptr<void> modelPtr, ImageResource&& imageResource) override;

  std::shared_ptr<common::AssetManager::VertexData> loadVertexDataInterleavingAsync(
      std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, IndexType indexSize,
      std::vector<common::BufferDescription>&& bufferDescriptions) override;

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
