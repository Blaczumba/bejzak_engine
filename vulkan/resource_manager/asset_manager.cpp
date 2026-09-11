#include "asset_manager.h"

#include <algorithm>
#include <format>
#include <functional>
#include <future>
#include <memory>
#include <numeric>
#include <span>
#include <tuple>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/util/index_buffer_util.h"

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(const LogicalDevice& logicalDevice, std::launch launchPolicy)
  : _logicalDevice(logicalDevice), _launchPolicy(launchPolicy),
    _freeImageDataIndices(MAX_STAGING_IMAGE_DATA_RESOURCES),
    _freeVertexDataIndices(MAX_STAGING_VERTEX_DATA_RESOURCES) {
  std::iota(_freeImageDataIndices.rbegin(), _freeImageDataIndices.rend(),
            StagingImageDataResourceHandle(0));
  std::iota(_freeVertexDataIndices.rbegin(), _freeVertexDataIndices.rend(),
            StagingVertexDataResourceHandle(0));
}

std::unique_ptr<AssetManager> AssetManager::create(
    const LogicalDevice& logicalDevice, std::launch launchPolicy) {
  return std::unique_ptr<AssetManager>(new AssetManager(logicalDevice, launchPolicy));
}

namespace {

lib::Buffer<VkBufferImageCopy> translateToVkBufferImageCopy(
    std::span<const ImageSubresource> imageSubresources, size_t stagingBufferOffset = 0) {
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

}  // namespace

StagingImageDataResourceHandle AssetManager::loadImageAsync(
    std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) {
  const StagingImageDataResourceHandle index = _freeImageDataIndices.back();
  _freeImageDataIndices.pop_back();
  _awaitingImageDataResources.emplace(
      index,
      std::async(_launchPolicy, [this, imageFunction = std::move(imageFunction)]() -> ImageData {
        const auto [resource, dataPtr] = imageFunction();
        ImageData imageData = {
          .stagingBuffer = BufferBuilder()
                               .withSize(resource.size)
                               .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                               .buildStagingBufferWithMetadata(_logicalDevice),
          .width = resource.width,
          .height = resource.height,
          .mipLevels = resource.mipLevels,
          .layerCount = resource.layerCount,
          .copyRegions = translateToVkBufferImageCopy(resource.subresources),
        };
        common::copyData(std::get<BufferMetadata>(imageData.stagingBuffer).getMappedMemoryAsSpan(),
                         0, std::span(static_cast<const std::byte*>(resource.data), resource.size));
        return imageData;
      }));
  return index;
}

// StagingImageDataResourceHandle AssetManager::loadImageAsync(
//     std::shared_ptr<void> modelPtr, std::span<const std::byte> data) {
//   const StagingImageDataResourceHandle index = _freeImageDataIndices.back();
//   _freeImageDataIndices.pop_back();
//   _awaitingImageDataResources.emplace(
//       index, std::async(_launchPolicy, [this, modelPtr = std::move(modelPtr), data]() ->
//       ImageData {
//         const auto [resource, dataPtr] = loadImage(data, "");  // TODO: refactor.
//         ImageData imageData = {
//           .stagingBuffer = BufferBuilder()
//                                .withSize(resource.size)
//                                .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
//                                .buildStagingBufferWithMetadata(_logicalDevice),
//           .width = resource.width,
//           .height = resource.height,
//           .mipLevels = resource.mipLevels,
//           .layerCount = resource.layerCount,
//           .copyRegions = translateToVkBufferImageCopy(resource.subresources),
//         };
//         common::copyData(std::get<BufferMetadata>(imageData.stagingBuffer).getMappedMemoryAsSpan(),
//                          0, std::span(static_cast<const std::byte*>(resource.data),
//                          resource.size));
//         return imageData;
//       }));
//   return index;
// }

StagingImageDataResourceHandle AssetManager::loadImageAsync(
    std::shared_ptr<void> modelPtr, ImageResource&& imageResource) {
  const StagingImageDataResourceHandle index = _freeImageDataIndices.back();
  _freeImageDataIndices.pop_back();
  _awaitingImageDataResources.emplace(
      index,
      std::async(
          _launchPolicy,
          [this, modelPtr = std::move(modelPtr),
           imageResource = std::move(imageResource)]() -> ImageData {
            ImageData imageData = {
              .stagingBuffer = BufferBuilder()
                                   .withSize(imageResource.size)
                                   .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                                   .buildStagingBufferWithMetadata(_logicalDevice),
              .width = imageResource.width,
              .height = imageResource.height,
              .mipLevels = imageResource.mipLevels,
              .layerCount = imageResource.layerCount,
              .copyRegions = translateToVkBufferImageCopy(imageResource.subresources),
            };
            common::copyData(
                std::get<BufferMetadata>(imageData.stagingBuffer).getMappedMemoryAsSpan(), 0,
                std::span(static_cast<const std::byte*>(imageResource.data), imageResource.size), 0,
                imageResource.size);
            return imageData;
          }));
  return index;
}

StagingVertexDataResourceHandle AssetManager::loadVertexDataInterleavingAsync(
    std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, uint8_t indexSize,
    std::vector<common::BufferDescription>&& bufferDescriptions) {
  const StagingVertexDataResourceHandle index = _freeVertexDataIndices.back();
  _freeVertexDataIndices.pop_back();
  _awaitingVertexDataResources.emplace(
      index,
      std::async(
          _launchPolicy,
          [this, modelPtr = std::move(modelPtr), indices, indexSize,
           bufferDescriptions = std::move(bufferDescriptions)]() mutable -> VertexData {
            VertexData vertexData;
            const VkPhysicalDeviceType deviceType =
                _logicalDevice.getPhysicalDevice().getPhysicalDeviceType();

            struct {
              VkBufferUsageFlags vertexBufferUsage = 0;
              VkBufferUsageFlags indexBufferUsage = 0;
            } flags;

            // For integrated graphics we create buffers properly in place so that they do
            // not need to be copied to the same memory later.
            if (deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
              flags.vertexBufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
              flags.indexBufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            } else if (deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
              flags.vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
              flags.indexBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            for (common::BufferDescription& description : bufferDescriptions) {
              auto [it, inserted] = vertexData.buffers.insert(
                  {std::move(description.name),
                   BufferBuilder()
                       .withSize(description.totalSize)
                       .withUsage(flags.vertexBufferUsage)
                       .buildStagingBufferWithMetadata(_logicalDevice)});
              common::copyDataInterleaving(
                  std::get<BufferMetadata>(it->second).getMappedMemoryAsSpan(),
                  description.attributes);
            }

            const IndexType shrunkIndexSize = getShrunkIndexSize(indices, getIndexType(indexSize));
            vertexData.indexBuffer =
                BufferBuilder()
                    .withSize(indices.size() / indexSize * static_cast<uint8_t>(shrunkIndexSize))
                    .withUsage(flags.indexBufferUsage)
                    .buildStagingBufferWithMetadata(_logicalDevice);
            common::copyAndShrinkIndexData(
                std::get<BufferMetadata>(vertexData.indexBuffer).getMappedMemoryAsSpan(), indices,
                static_cast<uint8_t>(shrunkIndexSize), indexSize);
            vertexData.indexType = [](IndexType indexType) {
              switch (indexType) {
                case IndexType::UINT8:
                  return VK_INDEX_TYPE_UINT8_EXT;
                case IndexType::UINT16:
                  return VK_INDEX_TYPE_UINT16;
                default:
                  return VK_INDEX_TYPE_UINT32;
              }
            }(shrunkIndexSize);
            return vertexData;
          }));

  return index;
}

const ImageData& AssetManager::getImageData(StagingImageDataResourceHandle index) {
  if (_imageDataResources.exists(*index)) [[likely]] {
    return _imageDataResources.getValue(*index);
  }

  auto it = _awaitingImageDataResources.find(index);
  if (it == _awaitingImageDataResources.cend()) [[unlikely]] {
    throw EngineException(std::format("Failed to find index {} in AssetManager.", *index));
  }

  const ImageData& data = _imageDataResources.insertUnsafe(*index, it->second.get());
  _awaitingImageDataResources.erase(it);
  return data;
}

ImageData AssetManager::releaseImageData(StagingImageDataResourceHandle index) {
  if (_imageDataResources.exists(*index)) [[likely]] {
    ImageData data = std::move(_imageDataResources.getValue(*index));
    _imageDataResources.eraseUnsafe(*index);
    return data;
  }

  auto it = _awaitingImageDataResources.find(index);
  if (it == _awaitingImageDataResources.cend()) [[unlikely]] {
    throw EngineException(std::format("Failed to find index {} in AssetManager.", *index));
  }

  ImageData data = std::move(_imageDataResources.insertUnsafe(*index, it->second.get()));
  _awaitingImageDataResources.erase(it);
  return data;
}

const VertexData& AssetManager::getVertexData(StagingVertexDataResourceHandle index) {
  if (_vertexDataResources.exists(*index)) [[likely]] {
    return _vertexDataResources.getValue(*index);
  }

  auto it = _awaitingVertexDataResources.find(index);
  if (it == _awaitingVertexDataResources.cend()) [[unlikely]] {
    throw EngineException(std::format("Failed to find index {} in AssetManager.", *index));
  }

  const VertexData& data = _vertexDataResources.insertUnsafe(*index, it->second.get());
  _awaitingVertexDataResources.erase(it);
  return data;
}

VertexData AssetManager::releaseVertexData(StagingVertexDataResourceHandle index) {
  if (_vertexDataResources.exists(*index)) [[likely]] {
    VertexData data = std::move(_vertexDataResources.getValue(*index));
    _vertexDataResources.eraseUnsafe(*index);
    return data;
  }

  auto it = _awaitingVertexDataResources.find(index);
  if (it == _awaitingVertexDataResources.cend()) [[unlikely]] {
    throw EngineException(std::format("Failed to find index {} in AssetManager.", *index));
  }

  VertexData data = std::move(_vertexDataResources.insertUnsafe(*index, it->second.get()));
  _awaitingVertexDataResources.erase(it);
  return data;
}

std::unique_ptr<NewAssetManager> NewAssetManager::create(
    const LogicalDevice& logicalDevice, BufferManager& bufferManager) {
  return std::unique_ptr<NewAssetManager>(new NewAssetManager(logicalDevice, bufferManager, 10));
}

NewAssetManager::~NewAssetManager() {
  {
    std::lock_guard lck(_mutex);
    _stop = true;
  }
  _conditionVariable.notify_all();
  for (auto& thread : _threads) {
    thread.thread.join();
  }
}

std::tuple<Ref<Buffer>, Ref<VirtualAllocation>, VirtualAllocationMetadata>
NewAssetManager::allocate(ThreadData& threadData, size_t size, size_t alignment, size_t blockSize) {
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

std::shared_ptr<NewAssetManager::ImageData> NewAssetManager::loadImageAsync(
    std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) {
  auto promise = std::make_shared<NewAssetManager::ImageData>();
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
      promise->loadState.store(LoadState::READY, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}

std::shared_ptr<NewAssetManager::ImageData> NewAssetManager::loadImageAsync(
    std::shared_ptr<void> modelPtr, ImageResource&& resource) {
  auto promise = std::make_shared<NewAssetManager::ImageData>();
  {
    std::lock_guard lock(_mutex);
    _tasks.push_back(
        [this, promise, modelPtr = std::move(modelPtr), resource = std::move(resource)](
            ThreadData& threadData, size_t blockSize, size_t alignment) {
          auto [stagingBufferRef, virtualAllocationRef, virtualAllocationMetadata] =
              allocate(threadData, resource.size, alignment, blockSize);

          common::copyData(
              std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory,
                        virtualAllocationMetadata.size),
              virtualAllocationMetadata.offset,
              std::span(static_cast<const std::byte*>(resource.data), resource.size));

          promise->stagingBuffer = std::move(stagingBufferRef);
          promise->virtualAllocation = std::move(virtualAllocationRef);
          promise->width = resource.width;
          promise->height = resource.height;
          promise->mipLevels = resource.mipLevels;
          promise->layerCount = resource.layerCount;
          promise->copyRegions = std::move(resource.subresources);
          promise->residentMips.store(0, std::memory_order_relaxed);
          promise->loadState.store(LoadState::READY, std::memory_order_release);
        });
  }
  _conditionVariable.notify_one();
  return promise;
}

std::shared_ptr<NewAssetManager::VertexData> NewAssetManager::loadVertexDataInterleavingAsync(
    std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, IndexType indexType,
    std::vector<common::BufferDescription>&& bufferDescriptions) {
  auto promise = std::make_shared<NewAssetManager::VertexData>();
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
          std::span(_bufferManager.getMetadata(stagingBufferRef.getHandle()).mappedMemory,
                    virtualAllocationMetadata.size),
          indices, static_cast<size_t>(shrunkIndexType), static_cast<size_t>(indexType),
          virtualAllocationMetadata.offset);

      promise->indexType = indexType;
      promise->indexBuffer =
          std::make_tuple(std::move(stagingBufferRef), std::move(virtualAllocationRef));
      promise->loadState.store(LoadState::READY, std::memory_order_release);
    });
  }
  _conditionVariable.notify_one();
  return promise;
}
