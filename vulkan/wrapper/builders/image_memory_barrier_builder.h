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

  VkImageMemoryBarrier2 build() const noexcept;

private:
  VkPipelineStageFlags2 _srcStageMask = {};
  VkAccessFlags2 _srcAccessMask = {};
  VkPipelineStageFlags2 _dstStageMask = {};
  VkAccessFlags2 _dstAccessMask = {};
  VkImageLayout _oldLayout = {};
  VkImageLayout _newLayout = {};
  uint32_t _srcQueueFamilyIndex = {};
  uint32_t _dstQueueFamilyIndex = {};
  VkImage _image = VK_NULL_HANDLE;
  VkImageSubresourceRange _subresourceRange = {};

  void* _pNext = nullptr;
};
