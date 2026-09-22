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

  VkImage getVkResource() const noexcept;

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
  VkImageAspectFlags imageAspect;

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

  ImageBuilder&& withAspect(VkImageAspectFlags aspect) && noexcept;

  ImageBuilder&& withMipLevels(uint32_t mipLevels) && noexcept;

  ImageBuilder&& withNumSamples(VkSampleCountFlagBits numSamples) && noexcept;

  ImageBuilder&& withTiling(VkImageTiling tiling) && noexcept;

  ImageBuilder&& withUsage(VkImageUsageFlags usage) && noexcept;

  ImageBuilder&& withLayerCount(uint32_t layerCount) && noexcept;

  ImageBuilder&& withFlags(VkImageCreateFlags flags) && noexcept;

  ImageMetadata buildMetadata() const noexcept;

  Image buildImage(const LogicalDevice& logicalDevice) const;

  std::tuple<Image, ImageMetadata> buildImageWithMetadata(const LogicalDevice& logicalDevice) const;

private:
  VkImageCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = VK_FORMAT_UNDEFINED,
    .extent = {1, 1, 1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VkImageAspectFlags _imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;

  void* _pNext = nullptr;
};

class ImageViewBuilder {
public:
  ImageViewBuilder& withFlags(VkImageViewCreateFlags flags) noexcept;

  ImageViewBuilder& withComponentMapping(VkComponentMapping components) noexcept;

  VkImageView buildAndAddToImage(Image& image, const ImageMetadata& metadata, uint32_t baseMipLevel,
                                 uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);

private:
  VkImageViewCreateFlags _flags = {};
  VkComponentMapping _components = {};
  VkImageSubresourceRange _subresourceRange = {};

  void* _pNext = nullptr;
};
