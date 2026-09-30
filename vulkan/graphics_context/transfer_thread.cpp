#include "vulkan/graphics_context/transfer_thread.h"

#include <expected>
#include <map>
#include <mutex>
#include <tuple>

#include "lib/types/memory.h"

namespace vlkn {
namespace {

constexpr size_t VERTEX_BUFFER_BLOCK_SIZE = 32 * lib::MiB;
constexpr size_t INDEX_BUFFER_BLOCK_SIZE_UINT8 = 4 * lib::MiB;
constexpr size_t INDEX_BUFFER_BLOCK_SIZE_UINT16 = 32 * lib::MiB;
constexpr size_t INDEX_BUFFER_BLOCK_SIZE_UINT32 = 8 * lib::MiB;
constexpr size_t THREAD_LOCAL_PROCESSING_SIZE = 32;
constexpr size_t BUDGET = 40 * lib::MiB;

constexpr size_t allocatorIndexFromIndexType(common::IndexType indexType) {
  switch (indexType) {
    case common::IndexType::UINT8:
      return 0;
    case common::IndexType::UINT16:
      return 1;
    default:
      return 2;
  }
}

}  // namespace

std::unique_ptr<TransferThread> TransferThread::create(
    const LogicalDevice& logicalDevice, BufferManager& bufferManager, ImageManager& imageManager) {
  return std::unique_ptr<TransferThread>(
      new TransferThread(logicalDevice, bufferManager, imageManager));
}

TransferThread::TransferThread(
    const LogicalDevice& logicalDevice, BufferManager& bufferManager, ImageManager& imageManager)
  : _logicalDevice(logicalDevice), _bufferManager(bufferManager), _imageManager(imageManager),
    _commandPool(CommandPoolBuilder()
                     .withQueueFamilyIndex(
                         *logicalDevice.getPhysicalDevice().getQueueFamilyIndices().transferFamily)
                     .withFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
                     .build(logicalDevice)),
    _commandBuffer(_commandPool->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY)),
    _timelineSemaphore(
        SemaphoreBuilder().withType(VK_SEMAPHORE_TYPE_TIMELINE).build(logicalDevice)),
    _fence(FenceBuilder().build(logicalDevice, VK_FENCE_CREATE_SIGNALED_BIT)) {}

CountingSemaphoreTransferIndex TransferThread::transferImageData(
    Ref<Image> imageRef, std::shared_ptr<common::AssetManager::ImageData> imageData) {
  CountingSemaphoreTransferIndex index(_nextIndex.fetch_add(1, std::memory_order_relaxed));
  _processingQueue.push_back(ImageProcessingState{
    .index = index, .imageRef = std::move(imageRef), .imageData = std::move(imageData)});
  return index;
}

CountingSemaphoreTransferIndex TransferThread::transferVertexData(
    Ref<Buffer> bufferRef, Ref<VirtualAllocation> virtualAllocationRef,
    std::shared_ptr<common::AssetManager::VertexData> vertexData) {
  CountingSemaphoreTransferIndex index(_nextIndex.fetch_add(1, std::memory_order_relaxed));
  _processingQueue.push_back(VertexProcessingState{
    .index = index,
    .bufferRef = std::move(bufferRef),
    .virtualAllocationRef = std::move(virtualAllocationRef),
    .vertexData = std::move(vertexData)});
  return index;
}

VkSemaphore TransferThread::getTimelineSemaphore() const noexcept {
  return _timelineSemaphore.getVkSemaphore();
}

std::tuple<Ref<Buffer>, Ref<VirtualAllocation>, VirtualAllocationMetadata> TransferThread::allocate(
    BufferAllocator& bufferAllocator, size_t bufferBlockSize, size_t size,
    VkBufferUsageFlags usage) {
  std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, VirtualAllocation::Error>
      expectedVirtualAllocation =
          bufferAllocator.bufferBlocks.back().virtualBlock.createVirtualAllocation(
              size, 1);  // the alignment is 1 because the data is tightly packed because of the
                         // same stride of all input data.
  if (!expectedVirtualAllocation.has_value()) {
    // Retry with the new buffer/block.
    if (bufferAllocator.blockToBeReclaimed.has_value()) {
      // Slower path: still very fast, if allocation didn't succeed then try to reuse the
      // retired block.
      bufferAllocator.bufferBlocks.push_back(std::move(*bufferAllocator.blockToBeReclaimed));
      bufferAllocator.blockToBeReclaimed.reset();
    } else {
      // The slowest path: allocate new staging buffer and virtual block for the allocation.
      auto [buffer, metadata] =
          BufferBuilder()
              .withUsage(usage)
              .withSize(std::max(VERTEX_BUFFER_BLOCK_SIZE, size))  // Rare situation when size is
                                                                   // bigger than blockSize.
              .buildVertexInputBufferWithMetadata(_logicalDevice);
      bufferAllocator.bufferBlocks.push_back(BufferAllocator::BufferBlock{
        .buffer = _bufferManager.storeBuffer(std::move(buffer), metadata),
        .virtualBlock = VirtualBlock::create(
            _logicalDevice.getMemoryAllocator(), std::max(VERTEX_BUFFER_BLOCK_SIZE, size))});
    }
    expectedVirtualAllocation =
        bufferAllocator.bufferBlocks.back().virtualBlock.createVirtualAllocation(size, 1);
    if (!expectedVirtualAllocation.has_value()) [[unlikely]] {
      throw EngineException("Failed to create virtual allocation.");
    }
  }
  auto& [virtualAllocation, virtualAllocationMetadata] = expectedVirtualAllocation.value();
  return std::make_tuple(bufferAllocator.bufferBlocks.back().buffer,
                         _virtualAllocationStrategy.transferResource(
                             std::move(virtualAllocation), virtualAllocationMetadata),
                         virtualAllocationMetadata);
}

void TransferThread::doWork() {
  // struct BudgetCalculator {
  //   size_t operator()(const ImageProcessingState& imageProcessingState) {
  //     return imageProces
  //   }

  //  size_t operator()(const VertexProcessingState& vertexProcessingState) {

  //  }
  //};
  // std::array<VkBufferCopy,
  // while (!_stop) {
  //  {
  //    std::unique_lock lck(_mutex);
  //    _conditionVariable.wait(lck, [this]() {
  //      return !_processingQueue.empty() || _stop;
  //    });

  //    if (_stop) [[unlikely]] {
  //      break;
  //    }

  //    size_t budget = 0;
  //    for (auto it = _processingQueue.cbegin(); it != _processingQueue.cend() && budget < BUDGET;
  //         it++) {
  //      budget += std::visit()
  //    }
  //  }

  //  // Here use
  //}
}

}  // namespace vlkn
