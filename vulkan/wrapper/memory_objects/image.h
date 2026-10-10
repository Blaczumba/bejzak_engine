#pragma once

#include <optional>
#include <span>
#include <tuple>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"

class Image {
  Image(const LogicalDevice& logicalDevice, VkImage image, Allocation&& allocation) noexcept;

public:
  Image() noexcept = default;

  static Image create(const LogicalDevice& logicalDevice, const VkImageCreateInfo& createInfo);

  Image(Image&& image) noexcept;

  Image& operator=(Image&& image) noexcept;

  ~Image();

  VkImage getVkImage() const noexcept;

  VkImage getUnderlyingResource() const noexcept;

  VkImageView getVkImageView(size_t index = 0) const noexcept;

  std::span<const VkImageView> getVkImageViews() const noexcept;

  const LogicalDevice* getLogicalDevice() const noexcept;

private:
  void addImageView(VkImageView imageView);

  void destroy();

  VkImage _image = VK_NULL_HANDLE;
  Allocation _allocation;
  std::vector<VkImageView> _views;

  const LogicalDevice* _logicalDevice = nullptr;

  friend class ImageViewBuilder;
};

struct ImageMetadata {
  VkImageCreateFlags imageCreateFlags;
  VkImageType imageType;
  VkFormat imageFormat;
  VkExtent3D imageExtent;
  uint32_t mipLevels;
  uint32_t arrayLayers;
  VkSampleCountFlagBits samples;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkSharingMode sharingMode;

  bool operator==(const ImageMetadata&) const = default;
};

class ImageBuilder {
public:
  ImageBuilder&& withType(VkImageType type) && noexcept;

  ImageBuilder&& withFormat(VkFormat format) && noexcept;

  ImageBuilder&& withExtent(uint32_t width) && noexcept;

  ImageBuilder&& withExtent(uint32_t width, uint32_t height) && noexcept;

  ImageBuilder&& withExtent(VkExtent2D extent) && noexcept;

  ImageBuilder&& withExtent(uint32_t width, uint32_t height, uint32_t depth) && noexcept;

  ImageBuilder&& withExtent(VkExtent3D extent) && noexcept;

  ImageBuilder&& withMipLevels(uint32_t mipLevels) && noexcept;

  ImageBuilder&& withNumSamples(VkSampleCountFlagBits numSamples) && noexcept;

  ImageBuilder&& withTiling(VkImageTiling tiling) && noexcept;

  ImageBuilder&& withUsage(VkImageUsageFlags usage) && noexcept;

  ImageBuilder&& withLayerCount(uint32_t layerCount) && noexcept;

  ImageBuilder&& withFlags(VkImageCreateFlags flags) && noexcept;

  ImageMetadata buildMetadata() const&& noexcept;

  Image buildImage(const LogicalDevice& logicalDevice) const&&;

  std::tuple<Image, ImageMetadata> buildImageWithMetadata(
      const LogicalDevice& logicalDevice) const&&;

private:
  ImageMetadata buildMetadataImpl() const noexcept;

  VkImageCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .extent = {1, 1, 1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT
  };
};

struct ImageViewMetadata {
  VkImageViewCreateFlags flags;
  VkImage image;
  VkImageViewType viewType;
  VkFormat format;
  VkComponentMapping components;
  VkImageSubresourceRange subresourceRange;
};

class ImageViewBuilder {
public:
  ImageViewBuilder() noexcept = default;

  ImageViewBuilder&& withViewType(VkImageViewType viewType) && noexcept;

  ImageViewBuilder&& withFlags(VkImageViewCreateFlags flags) && noexcept;

  ImageViewBuilder&& withFormat(VkFormat format) && noexcept;

  ImageViewBuilder&& withComponentMapping(VkComponentMapping components) && noexcept;

  ImageViewBuilder&& withSubresourceRange(
      VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount,
      uint32_t baseArrayLayer, uint32_t layerCount) && noexcept;

  ImageViewMetadata buildMetadata() const&& noexcept;

  VkImageView buildImageView(Image& image) &&;

  std::tuple<VkImageView, ImageViewMetadata> buildImageViewWithMetadata(Image& image) &&;

private:
  ImageViewMetadata buildMetadataImpl() const noexcept;

  VkImageViewCreateInfo _createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
};
