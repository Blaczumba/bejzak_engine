#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <span>
#include <thread>
#include <variant>

#include "common/abstractions/asset_manager.h"
#include "lib/types/strong_int.h"
#include "vulkan/resource_manager/buffer_manager.h"
#include "vulkan/resource_manager/image_manager.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/command_buffer/command_pool.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/synchronization/fence.h"
#include "vulkan/wrapper/synchronization/semaphore.h"

namespace vlkn {

DEFINE_STRONG_INT(CountingSemaphoreTransferIndex, size_t);

class TransferThread {
  TransferThread(
      const LogicalDevice& logicalDevice, BufferManager& bufferManager, ImageManager& imagemanager);

public:
  static std::unique_ptr<TransferThread> create(
      const LogicalDevice& logicalDevice, BufferManager& bufferManager, ImageManager& imageManager);

  ~TransferThread();

  CountingSemaphoreTransferIndex transferImageData(
      std::shared_ptr<common::AssetManager::ImageData> imageData);

  std::vector<CountingSemaphoreTransferIndex> transferImageData(
      std::span<std::shared_ptr<common::AssetManager::ImageData>> imageData);

  CountingSemaphoreTransferIndex transferVertexData(
      std::shared_ptr<common::AssetManager::VertexData> vertexData);

  VkSemaphore getTimelineSemaphore() const noexcept;

private:
  struct ImageProcessingState {
    CountingSemaphoreTransferIndex index;
    std::shared_ptr<common::AssetManager::ImageData> imageData;
    std::optional<uint32_t> processedMipLevel;
  };

  struct VertexProcessingState {
    CountingSemaphoreTransferIndex index;
    std::shared_ptr<common::AssetManager::VertexData> vertexData;
  };

  using ProcessingState = std::variant<ImageProcessingState, VertexProcessingState>;

  struct BufferAllocator {
    struct BufferBlock {
      Ref<Buffer> buffer;
      VirtualBlock virtualBlock;
    };
    std::deque<BufferBlock> bufferBlocks;  // We want pointer stability.
    std::optional<BufferBlock> blockToBeReclaimed;
  };

  void doWork();

  std::tuple<Ref<Buffer>, Ref<VirtualAllocation>, VirtualAllocationMetadata> allocate(
      BufferAllocator& bufferAllocator, size_t bufferBlockSize, size_t size,
      VkBufferUsageFlags usage);

  const LogicalDevice& _logicalDevice;
  BufferManager& _bufferManager;
  ImageManager& _imageManager;
  std::shared_ptr<CommandPool> _commandPool;
  CommandBuffer _commandBuffer;
  Semaphore _timelineSemaphore;
  Fence _fence;
  CountingSemaphoreTransferIndex _lastProcessedIndex;
  CountingSemaphoreTransferIndex _nextIndex = CountingSemaphoreTransferIndex(0);

  // Map whose keys are buffer strides.
  std::unordered_map<size_t, BufferAllocator> _vertexBufferAllocators;
  // There are 3 types of indexBuffer VK_INDEX_TYPE_UINT32, VK_INDEX_TYPE_UINT16 and
  // VK_INDEX_TYPE_UINT8_EXT
  std::array<BufferAllocator, 3> _indexBufferAllocators;
  AllocationStrategy<VirtualAllocation, AllocationPolicy::POOL_BASED> _virtualAllocationStrategy;

  std::thread _thread;
  std::deque<ProcessingState> _processingQueue;

  std::mutex _mutex;
  std::condition_variable _conditionVariable;
  bool _stop = false;
};

}  // namespace vlkn
