#include "vulkan/wrapper/memory_objects/image.h"

#include <cstdint>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_allocator/memory_allocator.h"
#include "vulkan/wrapper/util/check.h"

namespace {

template <typename T>
void chainExtendedField(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

struct ImageCreator {
  Allocation& allocation;
  const VkImageCreateInfo& imageCreateInfo;

  VkImage operator()(VmaWrapper& allocator) {
    VmaWrapper::Image imageData =
        allocator.createVkImage(imageCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    allocation = imageData.allocation;
    return imageData.image;
  }

  VkImage operator()(auto&&) {
    return VK_NULL_HANDLE;
  }
};

struct ImageDeleter {
  VkImage image;

  void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
    allocator.destroyVkImage(image, allocation);
  }

  void operator()(auto&&, auto&&) {}
};

}  // namespace

Image::Image(const LogicalDevice& logicalDevice, VkImage image, Allocation&& allocation) noexcept
  : _logicalDevice(&logicalDevice), _image(image), _allocation(std::move(allocation)) {}

Image Image::create(const LogicalDevice& logicalDevice, const VkImageCreateInfo& createInfo) {
  Allocation allocation;
  const VkImage image =
      std::visit(ImageCreator{allocation, createInfo}, logicalDevice.getMemoryAllocator());
  return Image(logicalDevice, image, std::move(allocation));
}

Image::Image(Image&& image) noexcept
  : _allocation(image._allocation), _image(std::exchange(image._image, VK_NULL_HANDLE)),
    _views(std::move(image._views)), _logicalDevice(image._logicalDevice) {}

Image& Image::operator=(Image&& image) noexcept {
  if (this == &image) [[unlikely]] {
    return *this;
  }

  if (_image != VK_NULL_HANDLE) {
    destroy();
  }

  _allocation = image._allocation;
  _image = std::exchange(image._image, VK_NULL_HANDLE);
  _views = std::move(image._views);
  _logicalDevice = image._logicalDevice;
  return *this;
}

void Image::destroy() {
  _logicalDevice->destroyResource([image = _image, allocation = std::move(_allocation),
                                   views = std::move(_views)](DestroyerContext context) {
    for (VkImageView view : views) {
      vkDestroyImageView(context.device, view, context.allocationCallbacks);
    }
    std::visit(ImageDeleter{image}, *context.memoryAllocator, allocation);
  });
}

Image::~Image() {
  if (_image != VK_NULL_HANDLE) {
    destroy();
    // TODO: Remove it once we enhance the collections of data.
    _image = VK_NULL_HANDLE;
  }
}

VkImage Image::getVkImage() const noexcept {
  return _image;
}

VkImage Image::getVkResource() const noexcept {
  return _image;
}

VkImageView Image::getVkImageView(size_t index) const noexcept {
  return index < _views.size() ? _views[index] : VK_NULL_HANDLE;
}

std::span<const VkImageView> Image::getVkImageViews() const noexcept {
  return _views;
}

const LogicalDevice* Image::getLogicalDevice() const noexcept {
  return _logicalDevice;
}

namespace {

VkImageViewType getImageViewType(VkImageType type, uint32_t layerCount, VkImageCreateFlags flags) {
  switch (type) {
    case VK_IMAGE_TYPE_1D:
      {
        if (layerCount > 1) {
          return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        }
        return VK_IMAGE_VIEW_TYPE_1D;
      }
    case VK_IMAGE_TYPE_2D:
      {
        if ((flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) && layerCount == 6) {
          return VK_IMAGE_VIEW_TYPE_CUBE;
        }
        if (layerCount > 1) {
          return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        return VK_IMAGE_VIEW_TYPE_2D;
      }
    case VK_IMAGE_TYPE_3D:
      return VK_IMAGE_VIEW_TYPE_3D;
  }
}

}  // namespace

void Image::addImageView(VkImageView imageView) {
  _views.push_back(imageView);
}

ImageBuilder&& ImageBuilder::withType(VkImageType type) && noexcept {
  _createInfo.imageType = type;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withFormat(VkFormat format) && noexcept {
  _createInfo.format = format;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withExtent(uint32_t width) && noexcept {
  _createInfo.extent = {width, 1, 1};
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withExtent(uint32_t width, uint32_t height) && noexcept {
  _createInfo.extent = {width, height, 1};
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withExtent(VkExtent2D extent) && noexcept {
  _createInfo.extent = {extent.width, extent.height, 1};
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withExtent(
    uint32_t width, uint32_t height, uint32_t depth) && noexcept {
  _createInfo.extent = {width, height, depth};
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withExtent(VkExtent3D extent) && noexcept {
  _createInfo.extent = extent;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withAspect(VkImageAspectFlags aspect) && noexcept {
  _imageAspect = aspect;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withMipLevels(uint32_t mipLevels) && noexcept {
  _createInfo.mipLevels = mipLevels;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withNumSamples(VkSampleCountFlagBits numSamples) && noexcept {
  _createInfo.samples = numSamples;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withTiling(VkImageTiling tiling) && noexcept {
  _createInfo.tiling = tiling;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withUsage(VkImageUsageFlags usage) && noexcept {
  _createInfo.usage = usage;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withLayerCount(uint32_t layerCount) && noexcept {
  _createInfo.arrayLayers = layerCount;
  return std::move(*this);
}

ImageBuilder&& ImageBuilder::withFlags(VkImageCreateFlags flags) && noexcept {
  _createInfo.flags = flags;
  return std::move(*this);
}

ImageMetadata ImageBuilder::buildMetadata() const noexcept {
  return ImageMetadata{
    .imageCreateFlags = _createInfo.flags,
    .imageType = _createInfo.imageType,
    .imageFormat = _createInfo.format,
    .imageExtent = _createInfo.extent,
    .mipLevels = _createInfo.mipLevels,
    .arrayLayers = _createInfo.arrayLayers,
    .samples = _createInfo.samples,
    .tiling = _createInfo.tiling,
    .usage = _createInfo.usage,
    .sharingMode = _createInfo.sharingMode,
    .imageAspect = _imageAspect,
  };
}

Image ImageBuilder::buildImage(const LogicalDevice& logicalDevice) const {
  return Image::create(logicalDevice, _createInfo);
}

std::tuple<Image, ImageMetadata> ImageBuilder::buildImageWithMetadata(
    const LogicalDevice& logicalDevice) const {
  return std::make_tuple(buildImage(logicalDevice), buildMetadata());
}

ImageViewBuilder& ImageViewBuilder::withFlags(VkImageViewCreateFlags flags) noexcept {
  _flags = flags;
  return *this;
}

ImageViewBuilder& ImageViewBuilder::withComponentMapping(VkComponentMapping components) noexcept {
  _components = components;
  return *this;
}

VkImageView ImageViewBuilder::buildAndAddToImage(
    Image& image, const ImageMetadata& metadata, uint32_t baseMipLevel, uint32_t levelCount,
    uint32_t baseArrayLayer, uint32_t layerCount) {
  if (image.getVkImage() == VK_NULL_HANDLE) {
    throw EngineException("VkImageView must be created from a valid Image.");
  }

  const VkImageViewCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = _pNext,
    .image = image.getVkImage(),
    .viewType = getImageViewType(metadata.imageType, layerCount, metadata.imageCreateFlags),
    .format = metadata.imageFormat,
    .subresourceRange = {.aspectMask = metadata.imageAspect,
                         .baseMipLevel = baseMipLevel,
                         .levelCount = levelCount,
                         .baseArrayLayer = baseArrayLayer,
                         .layerCount = layerCount}
  };
  VkImageView imageView;
  CHECK_VKCMD(
      vkCreateImageView(image.getLogicalDevice()->getVkDevice(), &createInfo, nullptr, &imageView),
      "Failed to create VkImageView.");
  image.addImageView(imageView);
  return imageView;
}
