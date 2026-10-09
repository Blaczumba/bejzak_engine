#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

class ImageMemoryBarrierBuilder {
public:
  ImageMemoryBarrierBuilder& withSrcMasks(
      VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept;

  ImageMemoryBarrierBuilder& withDstMasks(
      VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask) noexcept;

  ImageMemoryBarrierBuilder& withLayouts(VkImageLayout oldLayout, VkImageLayout newLayout) noexcept;

  ImageMemoryBarrierBuilder& withQueueFamilyIndices(
      uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) noexcept;

  ImageMemoryBarrierBuilder& withImage(
      VkImage image, const VkImageSubresourceRange& subresourceRange) noexcept;

  const VkImageMemoryBarrier2& build() const noexcept;

private:
  VkImageMemoryBarrier2 _imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
};
