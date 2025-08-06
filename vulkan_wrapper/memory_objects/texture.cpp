#include "texture.h"

#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/memory_objects/buffers.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <array>

Texture::Texture(const LogicalDevice& logicalDevice, VkImage image, const Allocation allocation,
    VkExtent3D extent, VkImageAspectFlags aspect, uint32_t mipLevels, uint32_t layerCount, VkImageLayout layout, VkImageView view, VkSampler sampler)
    : _logicalDevice(&logicalDevice), _image(image), _allocation(allocation),
    _layout(layout), _view(view), _sampler(sampler), _extent(extent), _aspect(aspect), _mipLevels(mipLevels), _layerCount(layerCount) { }

Texture::Texture() : _image(VK_NULL_HANDLE), _view(VK_NULL_HANDLE), _sampler(VK_NULL_HANDLE) {}

Texture::Texture(Texture&& texture) noexcept
    : _allocation(texture._allocation), _image(std::exchange(texture._image, VK_NULL_HANDLE)), 
    _view(std::exchange(texture._view, VK_NULL_HANDLE)), _sampler(std::exchange(texture._sampler, VK_NULL_HANDLE)),
    _layout(texture._layout), _logicalDevice(texture._logicalDevice), _extent(texture._extent), _aspect(texture._aspect),
    _mipLevels(texture._mipLevels), _layerCount(texture._layerCount) { }

Texture& Texture::operator=(Texture&& texture) noexcept {
    if (this == &texture) {
        return *this;
    }
    _allocation = texture._allocation;
    _image = std::exchange(texture._image, VK_NULL_HANDLE);
    _view = std::exchange(texture._view, VK_NULL_HANDLE);
    _sampler = std::exchange(texture._sampler, VK_NULL_HANDLE);
    _layout = texture._layout;
    _extent = texture._extent;
    _aspect = texture._aspect;
    _mipLevels = texture._mipLevels;
    _layerCount = texture._layerCount;
    _logicalDevice = texture._logicalDevice;
    return *this;
}

namespace {

struct ImageCreator {
    Allocation& allocation;
    const ImageParameters& params;

    const ErrorOr<VkImage> operator()(VmaWrapper& allocator) {
        ASSIGN_OR_RETURN(VmaWrapper::Image imageData, allocator.createVkImage(params, VK_IMAGE_LAYOUT_UNDEFINED, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
        allocation = imageData.allocation;
        return imageData.image;
    }

    const ErrorOr<VkImage> operator()(auto&&) {
        return Error(EngineError::NOT_RECOGNIZED_TYPE);
    }
};

struct ImageDeleter {
    VkImage image;

    void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
        allocator.destroyVkImage(image, allocation);
    }

    void operator()(auto&&, auto&&) {}
};

} // namespace

Texture::~Texture() {
    const VkDevice device = _logicalDevice->getVkDevice();
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _sampler, nullptr);
    }
    if (_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, _view, nullptr);
    }
    if (_image != VK_NULL_HANDLE) {
        std::visit(ImageDeleter{ _image }, _logicalDevice->getMemoryAllocator(), _allocation);
    }
}

VkImage Texture::getVkImage() const {
    return _image;
}

VkImageView Texture::getVkImageView() const {
    return _view;
}

VkSampler Texture::getVkSampler() const {
    return _sampler;
}

VkExtent2D Texture::getVkExtent2D() const {
    return VkExtent2D{ _extent.width, _extent.height };
}

VkExtent3D Texture::getVkExtent3D() const {
    return _extent;
}

VkImageLayout Texture::getVkImageLayout() const {
    return _layout;
}

void Texture::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
    transitionImageLayout(commandBuffer, _image, _layout, newLayout, _aspect, _mipLevels, _layerCount);
    _layout = newLayout;
}

static inline ErrorOr<VkImage> allocate(Allocation& allocation, const ImageParameters& imageParameters, MemoryAllocator& memoryAllocator) {
    return std::visit(ImageCreator{ allocation, imageParameters }, memoryAllocator);
}

TextureBuilder& TextureBuilder::withLayout(VkImageLayout layout) {
    _imageLayout = layout;
    return *this;
}

TextureBuilder& TextureBuilder::withFormat(VkFormat format) {
    _imageParameters.format = format;
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(uint32_t width) {
    _imageParameters.extent = { width, 1, 1 };
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(uint32_t width, uint32_t height) {
    _imageParameters.extent = { width, height, 1 };
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(VkExtent2D extent) {
    _imageParameters.extent = { extent.width, extent.height, 1 };
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(uint32_t width, uint32_t height, uint32_t depth) {
    _imageParameters.extent = { width, height, depth };
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(VkExtent3D extent) {
    _imageParameters.extent = extent;
    return *this;
}

TextureBuilder& TextureBuilder::withAspect(VkImageAspectFlags aspect) {
    _imageParameters.aspect = aspect;
    return *this;
}

TextureBuilder& TextureBuilder::withMipLevels(uint32_t mipLevels) {
    _imageParameters.mipLevels = mipLevels;
    return *this;
}

TextureBuilder& TextureBuilder::withNumSamples(VkSampleCountFlagBits numSamples) {
    _imageParameters.numSamples = numSamples;
    return *this;
}

TextureBuilder& TextureBuilder::withTiling(VkImageTiling tiling) {
    _imageParameters.tiling = tiling;
    return *this;
}

TextureBuilder& TextureBuilder::withUsage(VkImageUsageFlags usage) {
    _imageParameters.usage = usage;
    return *this;
}

TextureBuilder& TextureBuilder::withProperties(VkMemoryPropertyFlags properties) {
    _imageParameters.properties = properties;
    return *this;
}

TextureBuilder& TextureBuilder::withLayerCount(uint32_t layerCount) {
    _imageParameters.layerCount = layerCount;
    return *this;
}

TextureBuilder& TextureBuilder::withMagFilter(VkFilter magFilter) {
    _samplerParameters.magFilter = magFilter;
    return *this;
}

TextureBuilder& TextureBuilder::withMinFilter(VkFilter minFilter) {
    _samplerParameters.minFilter = minFilter;
    return *this;
}

TextureBuilder& TextureBuilder::withMipmapMode(VkSamplerMipmapMode mipmapMode) {
    _samplerParameters.mipmapMode = mipmapMode;
    return *this;
}

TextureBuilder& TextureBuilder::withAddressModes(VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW) {
    _samplerParameters.addressModeU = addressModeU;
    _samplerParameters.addressModeV = addressModeV;
    _samplerParameters.addressModeW = addressModeW;
    return *this;
}

TextureBuilder& TextureBuilder::withMipLodBias(float mipLodBias) {
    _samplerParameters.mipLodBias = mipLodBias;
    return *this;
}

TextureBuilder& TextureBuilder::withMaxAnisotropy(float maxAnisotropy) {
    _samplerParameters.maxAnisotropy = maxAnisotropy;
    return *this;
}

TextureBuilder& TextureBuilder::withCompareOp(VkCompareOp compareOp) {
    _samplerParameters.compareOp = compareOp;
    return *this;
}

TextureBuilder& TextureBuilder::withMinLod(float minLod) {
    _samplerParameters.minLod = minLod;
    return *this;
}

TextureBuilder& TextureBuilder::withMaxLod(float maxLod) {
    _samplerParameters.maxLod = maxLod;
    return *this;
}

TextureBuilder& TextureBuilder::withBorderColor(VkBorderColor borderColor) {
    _samplerParameters.borderColor = borderColor;
    return *this;
}

TextureBuilder& TextureBuilder::withUnnormalizedCoordinates(VkBool32 unnormalizedCoordinates) {
    _samplerParameters.unnormalizedCoordinates = unnormalizedCoordinates;
    return *this;
}

ErrorOr<Texture> TextureBuilder::buildAttachment(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, _imageLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
    return Texture(logicalDevice, image, allocation, _imageParameters.extent, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount, _imageLayout, view);
}

ErrorOr<Texture> TextureBuilder::buildImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::span<const VkBufferImageCopy> copyRegions) const {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _imageLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
    ASSIGN_OR_RETURN(const VkSampler sampler, logicalDevice.createSampler(_samplerParameters));
    return Texture(logicalDevice, image, allocation, _imageParameters.extent, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount, _imageLayout, view, sampler);
}

ErrorOr<Texture> TextureBuilder::buildImageSampler(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const {
	Allocation allocation;
	ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
	transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, _imageLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
	const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
	ASSIGN_OR_RETURN(const VkSampler sampler, logicalDevice.createSampler(_samplerParameters));
	return Texture(logicalDevice, image, allocation, _imageParameters.extent, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount, _imageLayout, view, sampler);
}

ErrorOr<Texture> TextureBuilder::buildMipmapImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, std::span<const VkBufferImageCopy> copyRegions) const {
	Allocation allocation;
	ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
	transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
	copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
	generateImageMipmaps(commandBuffer, image, _imageParameters.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		_imageParameters.extent.width, _imageParameters.extent.height, _imageParameters.mipLevels, _imageParameters.layerCount);
	const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
	ASSIGN_OR_RETURN(const VkSampler sampler, logicalDevice.createSampler(_samplerParameters));
	return Texture(logicalDevice, image, allocation, _imageParameters.extent, _imageParameters.aspect, _imageParameters.mipLevels,
        _imageParameters.layerCount, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, view, sampler);
}
