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

#include "common/buffer/buffer_utils.h"
#include "common/buffer/index_buffer_utils.h"
#include "common/model_loader/image_loader/types.h"
#include "common/util/asset_manager.h"
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
               uint8_t threadCount, size_t size);

  void doWork(uint8_t threadIndex);

  void cleanVirtualAllocatorCounters(ThreadData& threadData);

  void cleanVirtualBlocks(ThreadData& threadData);

public:
  static std::unique_ptr<AssetManager> create(
      const LogicalDevice& logicalDevice, BufferManager& bufferManager);

  ~AssetManager();

  std::shared_ptr<common::AssetManager::ImageData> loadImageAsync(
      std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) override;

  std::shared_ptr<common::AssetManager::ImageData> loadImageAsync(
      std::shared_ptr<void> modelPtr, ImageResource&& imageResource) override;

  std::shared_ptr<common::AssetManager::VertexData> loadVertexDataInterleavingAsync(
      std::shared_ptr<void> modelPtr, std::span<const std::byte> indices,
      common::IndexType indexSize,
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
