#include "vulkan/wrapper/builders/image_memory_barrier_builder.h"

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withSrcMasks(
    VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept {
  _imageMemoryBarrier.srcStageMask = stageMask;
  _imageMemoryBarrier.srcAccessMask = accessMask;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withDstMasks(
    VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept {
  _imageMemoryBarrier.dstStageMask = stageMask;
  _imageMemoryBarrier.dstAccessMask = accessMask;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withLayouts(
    VkImageLayout oldLayout, VkImageLayout newLayout) noexcept {
  _imageMemoryBarrier.oldLayout = oldLayout;
  _imageMemoryBarrier.newLayout = newLayout;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withQueueFamilyIndices(
    uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) noexcept {
  _imageMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
  _imageMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withImage(
    VkImage image, const VkImageSubresourceRange& subresourceRange) noexcept {
  _imageMemoryBarrier.image = image;
  _imageMemoryBarrier.subresourceRange = subresourceRange;
  return *this;
}

const VkImageMemoryBarrier2& ImageMemoryBarrierBuilder::build() const noexcept {
  return _imageMemoryBarrier;
}
