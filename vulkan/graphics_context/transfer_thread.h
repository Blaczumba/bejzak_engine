#pragma once

#include <array>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <span>
#include <thread>
#include <variant>

#include "common/abstractions/asset_manager.h"
#include "lib/buffer/buffer.h"
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
  TransferThread(const LogicalDevice& logicalDevice, VkQueue transferingQueue,
                 uint32_t transferQueueFamilyIndex, uint32_t universalQueueFamilyIndex);

public:
  static std::unique_ptr<TransferThread> create(const LogicalDevice& logicalDevice);

  ~TransferThread();

  CountingSemaphoreTransferIndex transferImageData(
      Ref<Image> imageRef, std::shared_ptr<common::AssetManager::ImageData>);

  std::vector<CountingSemaphoreTransferIndex> transferImageData(
      std::span<std::shared_ptr<common::AssetManager::ImageData>> imageData);

  CountingSemaphoreTransferIndex transferVertexData(
      Ref<Buffer> bufferRef, Ref<VirtualAllocation> virtualAllocationRef,
      std::shared_ptr<common::AssetManager::VertexData> vertexData);

  VkSemaphore getTimelineSemaphore() const noexcept;

private:
  static constexpr size_t TRANSFERS_IN_FLIGHT = 2;

  struct ImageProcessingState {
    CountingSemaphoreTransferIndex index;
    Ref<Image> imageRef;
    VkImageAspectFlags aspect;
    VkImageLayout dstLayout;
    std::shared_ptr<common::AssetManager::ImageData> imageData;
    std::optional<uint32_t> processedMipLevel;
  };

  struct VertexProcessingState {
    CountingSemaphoreTransferIndex index;
    lib::Buffer<std::pair<Ref<Buffer>, Ref<VirtualAllocation>>> vertexBuffers;
    Ref<Buffer> indexBufferRef;
    Ref<VirtualAllocation> indexBufferVirtualAllocationRef;
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

  const LogicalDevice& _logicalDevice;
  VkQueue _transferQueue;
  uint32_t _transferQueueFamilyIndex, _universalQueueFamilyIndex;
  std::shared_ptr<CommandPool> _commandPool;
  std::array<std::tuple<CommandBuffer, Fence>, TRANSFERS_IN_FLIGHT> _inFlightObjects;
  Semaphore _timelineSemaphore;
  CountingSemaphoreTransferIndex _nextIndex = CountingSemaphoreTransferIndex(0);
  CountingSemaphoreTransferIndex _processedIndex = CountingSemaphoreTransferIndex(0);

  bool _stop = false;
  std::thread _thread;
  std::deque<ProcessingState> _processingQueue;
  uint8_t _currentTransfer = 0;

  std::atomic<bool> _jobsNotEmpty = false;
  std::mutex _mutex;
  std::condition_variable _conditionVariable;
  std::deque<ProcessingState> _pushedJobs;
};

}  // namespace vlkn
