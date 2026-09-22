#include "vulkan/wrapper/builders/image_memory_barrier_builder.h"

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withSrcMasks(
    VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept {
  _srcStageMask = stageMask;
  _srcAccessMask = accessMask;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withDstMasks(
    VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept {
  _dstStageMask = stageMask;
  _dstAccessMask = accessMask;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withLayouts(
    VkImageLayout oldLayout, VkImageLayout newLayout) noexcept {
  _oldLayout = oldLayout;
  _newLayout = newLayout;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withQueueFamilyIndices(
    uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) noexcept {
  _srcQueueFamilyIndex = srcQueueFamilyIndex;
  _dstQueueFamilyIndex = dstQueueFamilyIndex;
  return *this;
}

ImageMemoryBarrierBuilder& ImageMemoryBarrierBuilder::withImage(
    VkImage image, const VkImageSubresourceRange& subresourceRange) noexcept {
  _image = image;
  _subresourceRange = subresourceRange;
  return *this;
}

VkImageMemoryBarrier2 ImageMemoryBarrierBuilder::build() const noexcept {
  return VkImageMemoryBarrier2{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .pNext = _pNext,
    .srcStageMask = _srcStageMask,
    .srcAccessMask = _srcAccessMask,
    .dstStageMask = _dstStageMask,
    .dstAccessMask = _dstAccessMask,
    .oldLayout = _oldLayout,
    .newLayout = _newLayout,
    .srcQueueFamilyIndex = _srcQueueFamilyIndex,
    .dstQueueFamilyIndex = _dstQueueFamilyIndex,
    .image = _image,
    .subresourceRange = _subresourceRange};
}
