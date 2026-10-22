#include "vulkan/graphics_context/transfer_thread.h"

#include <cassert>
#include <expected>
#include <map>
#include <mutex>
#include <tuple>
#include <ranges>

#include "lib/types/memory.h"

namespace vlkn {
namespace {

constexpr size_t VERTEX_BUFFER_BLOCK_SIZE = 64 * lib::MiB;
constexpr size_t INDEX_BUFFER_BLOCK_SIZE = 16 * lib::MiB;
constexpr size_t THREAD_LOCAL_PROCESSING_SIZE = 16;
constexpr size_t BUDGET = 20 * lib::MiB;

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

lib::Buffer<VkBufferImageCopy> translateImageSubresourcesToVkBufferImageCopy(
    std::span<const ImageSubresource> imageSubresources, size_t stagingBufferOffset) {
  lib::Buffer<VkBufferImageCopy> vkSubresources(imageSubresources.size());
  std::transform(
      std::cbegin(imageSubresources), std::cend(imageSubresources), vkSubresources.begin(),
      [stagingBufferOffset](const ImageSubresource& subresource) {
        return VkBufferImageCopy{
          .bufferOffset = subresource.offset + stagingBufferOffset,
          .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                               .mipLevel = subresource.mipLevel,
                               .baseArrayLayer = subresource.baseArrayLayer,
                               .layerCount = subresource.layerCount},
          .imageExtent = {.width = subresource.width,
                               .height = subresource.height,
                               .depth = subresource.depth},
        };
      });
  return vkSubresources;
}

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace

std::unique_ptr<TransferThread> TransferThread::create(const LogicalDevice& logicalDevice) {
  VkQueue transferQueue = logicalDevice.getDedicatedTransferVkQueue();
  std::optional<uint32_t> transferQueueFamilyIndex =
      logicalDevice.getPhysicalDevice().getQueueFamilyIndices().dedicatedTransferFamily;
  uint32_t universalQueueFamilyIndex =
      logicalDevice.getPhysicalDevice().getQueueFamilyIndices().universalFamily;
  if (!transferQueueFamilyIndex.has_value()) {
    transferQueue = logicalDevice.getUniversalVkQueue();
    transferQueueFamilyIndex = universalQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  }
  assert(transferQueue != VK_NULL_HANDLE);
  return std::unique_ptr<TransferThread>(new TransferThread(
      logicalDevice, transferQueue, *transferQueueFamilyIndex, universalQueueFamilyIndex));
}

TransferThread::TransferThread(const LogicalDevice& logicalDevice, VkQueue transferQueue, uint32_t transferQueueFamilyIndex, uint32_t universalQueueFamilyIndex)
  : _logicalDevice(logicalDevice), _transferQueue(transferQueue),
    _transferQueueFamilyIndex(transferQueueFamilyIndex),
    _universalQueueFamilyIndex(universalQueueFamilyIndex),
    _commandPool(CommandPoolBuilder()
                     .withQueueFamilyIndex(*logicalDevice.getPhysicalDevice()
                                                .getQueueFamilyIndices()
                                                .dedicatedTransferFamily)  // TODO: Need to check
                                                                           // for support first.
                     .withFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
                     .build(logicalDevice)),
    _timelineSemaphore(
        SemaphoreBuilder().withType(VK_SEMAPHORE_TYPE_TIMELINE).build(logicalDevice)) {
  for (auto& [commandBuffer, fence] : _inFlightObjects) {
    commandBuffer = _commandPool->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    fence = FenceBuilder().build(logicalDevice, VK_FENCE_CREATE_SIGNALED_BIT);
  }
}

CountingSemaphoreTransferIndex TransferThread::transferImageData(
    Ref<Image> imageRef, std::shared_ptr<common::AssetManager::ImageData> imageData) {
        {
          std::lock_guard<std::mutex> lck(_mutex);
          _pushedJobs.push_back(ImageProcessingState{
            .index = _nextIndex, .imageRef = std::move(imageRef), .imageData = std::move(imageData)});
        }
  _conditionVariable.notify_one();
  _jobsNotEmpty.store(true, std::memory_order_relaxed);
  return _nextIndex++;
}

CountingSemaphoreTransferIndex TransferThread::transferVertexData(
    Ref<Buffer> bufferRef, Ref<VirtualAllocation> virtualAllocationRef,
    std::shared_ptr<common::AssetManager::VertexData> vertexData) {
        {
          std::lock_guard<std::mutex> lck(_mutex);
          _pushedJobs.push_back(VertexProcessingState{
            .index = _nextIndex,
            //.bufferRef = std::move(bufferRef),
            //.virtualAllocationRef = std::move(virtualAllocationRef),
            .vertexData = std::move(vertexData)});
        }
  _conditionVariable.notify_one();
  _jobsNotEmpty.store(true, std::memory_order_relaxed);
  return _nextIndex++;
}

VkSemaphore TransferThread::getTimelineSemaphore() const noexcept {
  return _timelineSemaphore.getVkSemaphore();
}

void TransferThread::doWork() {
  std::deque<ProcessingState> processingCache;
  while (!_stop) {
    {
      {
        std::unique_lock lck(_mutex);
        _conditionVariable.wait(lck, [this]() {
          return !_pushedJobs.empty() || _stop;
        });

        if (_stop) [[unlikely]] {
          break;
        }
        processingCache = std::move(_pushedJobs);
      }
      _processingQueue.append_range(std::views::as_rvalue(processingCache));
      processingCache.clear();

      bool iteratedAtLeastOnce = false;
      //while ((!_jobsNotEmpty.load(std::memory_order_relaxed) || !iteratedAtLeastOnce) && !_processingQueue.empty()) {
      //  iteratedAtLeastOnce = true;
      //  size_t budget = 0;
      //  bool shouldSubmit = false;
      //  auto& [commandBuffer, fence] = _inFlightObjects[_currentTransfer];
      //  fence.wait();
      //  commandBuffer.resetCommandBuffer();
      //  for (auto it = _processingQueue.begin(); it != _processingQueue.end() && budget < BUDGET; it++) {
      //    auto [transferedMemory, isProcessed] = std::visit(
      //        Overloaded{
      //          [this, &commandBuffer](const VertexProcessingState& vertexProcessingState)
      //              -> std::tuple<size_t, bool> {
      //            size_t budget = 0;
      //            if (vertexProcessingState.vertexData->loadState.load(std::memory_order_acquire)
      //                != common::AssetManager::LoadState::FINISHED) {
      //              return {budget, false};
      //            }
      //            const VirtualAllocationMetadata& indexBufferVirtualAllocationMetadata =
      //                WeakRef<VirtualAllocation>(
      //                    std::get<1>(vertexProcessingState.vertexData->indexBuffer))
      //                    .getMetadata();
      //            auto [gpuIndexBuffer, gpuIndexBufferVirtualAllocation,
      //                  gpuIndexBufferVirtualAllocationMetadata] =
      //                allocate(_indexBufferAllocators[allocatorIndexFromIndexType(
      //                             vertexProcessingState.vertexData->indexType)],
      //                         INDEX_BUFFER_BLOCK_SIZE, indexBufferVirtualAllocationMetadata.size,
      //                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
      //            commandBuffer.copyBufferToBuffer(
      //                WeakRef<Buffer>(std::get<0>(vertexProcessingState.vertexData->indexBuffer))
      //                    .getUnderlyingResource(),
      //                gpuIndexBuffer.getUnderlyingResource(),
      //                {
      //                  VkBufferCopy{.srcOffset = indexBufferVirtualAllocationMetadata.offset,
      //                               .dstOffset = gpuIndexBufferVirtualAllocationMetadata.offset,
      //                               .size = indexBufferVirtualAllocationMetadata.size}
      //            });
      //            budget += indexBufferVirtualAllocationMetadata.size;

      //            for (const auto& [vertexBuffer, virtualAllocation, stride] :
      //                 std::views::values(vertexProcessingState.vertexData->buffers)) {
      //              const VirtualAllocationMetadata& vertexBufferVirtualAllocationMetadata =
      //                  WeakRef<VirtualAllocation>(virtualAllocation).getMetadata();
      //              auto [gpuVertexBuffer, gpuVertexBufferVirtualAllocation,
      //                    gpuVertexBufferVirtualAllocationMetadata] =
      //                  allocate(
      //                      _vertexBufferAllocators[stride], VERTEX_BUFFER_BLOCK_SIZE,
      //                      vertexBufferVirtualAllocationMetadata.size,
      //                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
      //              commandBuffer.copyBufferToBuffer(
      //                  WeakRef<Buffer>(vertexBuffer).getUnderlyingResource(),
      //                  gpuVertexBuffer.getUnderlyingResource(),
      //                  {
      //                    VkBufferCopy{.srcOffset = vertexBufferVirtualAllocationMetadata.offset,
      //                                 .dstOffset = gpuVertexBufferVirtualAllocationMetadata.offset,
      //                                 .size = vertexBufferVirtualAllocationMetadata.size}
      //              });
      //              budget += vertexBufferVirtualAllocationMetadata.size;
      //            }
      //            return {budget, true};
      //          },
      //          [this, &commandBuffer](
      //              ImageProcessingState& imageProcessingState) -> std::tuple<size_t, bool> {
      //            if (imageProcessingState.imageData->loadState.load(std::memory_order_acquire)
      //                != common::AssetManager::LoadState::FINISHED) {
      //              return {0, false};
      //            }

      //            auto [image, imageMetadata] =
      //                imageProcessingState.imageRef.getUnderlyingResourceWithMetadata();
      //            commandBuffer.transitionImageLayout(
      //                image, imageProcessingState.aspect, VK_IMAGE_LAYOUT_UNDEFINED,
      //                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      //                imageMetadata.arrayLayers);
      //            const VirtualAllocationMetadata& virtualAllcoationMetadata =
      //                WeakRef<VirtualAllocation>(imageProcessingState.imageData->virtualAllocation)
      //                    .getMetadata();
      //            
      //            // TODO;
      //            imageProcessingState.processedMipLevel;
      //            imageMetadata.mipLevels;
      //            imageProcessingState.imageData->residentMips;
      //            size_t copyRegionsSize = imageProcessingState.imageData->copyRegions.data();
      //            std::span<const ImageSubresource> copyRegions(
      //                imageProcessingState.imageData->copyRegions.data() + copyRegionsSize - residentMips, residentMips - processedMips);


      //            commandBuffer.copyBufferToImage(
      //                WeakRef<Buffer>(imageProcessingState.imageData->stagingBuffer)
      //                    .getUnderlyingResource(),
      //                image,
      //                translateImageSubresourcesToVkBufferImageCopy(
      //                    imageProcessingState.imageData->copyRegions,
      //                    virtualAllcoationMetadata.offset));
      //            commandBuffer.transitionImageLayout(
      //                image, imageProcessingState.aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      //                imageProcessingState.dstLayout, 0, imageMetadata.mipLevels, 0,
      //                imageMetadata.arrayLayers, _transferQueueFamilyIndex,
      //                _universalQueueFamilyIndex);
      //            return {virtualAllcoationMetadata.size, true};
      //          }},
      //        *it);
      //    shouldSubmit |= (transferedMemory != 0);
      //    budget += transferedMemory;
      //  }

      //  // Submit the CommandBuffer to queue...
      //  _currentTransfer = (_currentTransfer == TRANSFERS_IN_FLIGHT) ? 0 : _currentTransfer + 1;
      //}
    }

    // Here use
  }
}

}  // namespace vlkn
