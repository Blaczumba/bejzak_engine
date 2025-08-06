#include "texture.h"

#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/memory_objects/buffers.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <array>

Texture::Texture(const LogicalDevice& logicalDevice, VkImage image, const Allocation allocation,
    VkImageLayout layout, const ImageParameters& imageParameters, VkImageView view, VkSampler sampler,
    const SamplerParameters& samplerParameters)
    : _logicalDevice(&logicalDevice), _image(image), _allocation(allocation), _layout(layout),
    _imageParameters(imageParameters), _view(view), _sampler(sampler), _samplerParameters(samplerParameters) {

}

Texture::Texture() : _image(VK_NULL_HANDLE), _view(VK_NULL_HANDLE), _sampler(VK_NULL_HANDLE) {}

Texture::Texture(Texture&& texture) noexcept
    : _allocation(texture._allocation), _image(std::exchange(texture._image, VK_NULL_HANDLE)), 
    _view(std::exchange(texture._view, VK_NULL_HANDLE)), _sampler(std::exchange(texture._sampler, VK_NULL_HANDLE)),
    _layout(texture._layout), _imageParameters(texture._imageParameters), _samplerParameters(texture._samplerParameters),
    _logicalDevice(texture._logicalDevice) {

}

Texture& Texture::operator=(Texture&& texture) noexcept {
    if (this == &texture) {
        return *this;
    }
    _allocation = texture._allocation;
    _image = std::exchange(texture._image, VK_NULL_HANDLE);
    _view = std::exchange(texture._view, VK_NULL_HANDLE);
    _sampler = std::exchange(texture._sampler, VK_NULL_HANDLE);
    _layout = texture._layout;
    _imageParameters = texture._imageParameters;
    _samplerParameters = texture._samplerParameters;
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

ErrorOr<Texture> Texture::create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format) {
    const ImageParameters imageParams = {
        .format = format,
        .width = width,
        .height = height,
        .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };
    const SamplerParameters samplerParams = {
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    };
    return createImageSampler(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams, samplerParams);
}

ErrorOr<Texture> Texture::create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
    const ImageParameters imageParams = {
        .format = format,
        .width = dimensions.width,
        .height = dimensions.height,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevels = dimensions.mipLevels,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .layerCount = 1,
    };
    const SamplerParameters samplerParams = {
        .maxAnisotropy = samplerAnisotropy,
        .maxLod = static_cast<float>(dimensions.mipLevels)
    };
    return createMipmapImage(logicalDevice, commandBuffer, stagingBuffer, dimensions.copyRegions, imageParams, samplerParams);
}

ErrorOr<Texture> Texture::createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
    const ImageParameters imageParams = {
        .format = format,
        .width = dimensions.width,
        .height = dimensions.height,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevels = dimensions.mipLevels,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .layerCount = 6,
    };
    const SamplerParameters samplerParams = {
        .maxAnisotropy = samplerAnisotropy,
        .maxLod = static_cast<float>(dimensions.mipLevels)
    };
    return createImage(logicalDevice, commandBuffer, stagingBuffer, dimensions.copyRegions, imageParams, samplerParams);
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
    return VkExtent2D{ _imageParameters.width, _imageParameters.height };
}

VkImageLayout Texture::getVkImageLayout() const {
    return _layout;
}

void Texture::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
    transitionImageLayout(commandBuffer, _image, _layout, newLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    _layout = newLayout;
}

const ImageParameters& Texture::getImageParameters() const {
    return _imageParameters;
}

const SamplerParameters& Texture::getSamplerParameters() const {
    return _samplerParameters;
}

static inline ErrorOr<VkImage> allocate(Allocation& allocation, const ImageParameters& imageParameters, MemoryAllocator& memoryAllocator) {
    return std::visit(ImageCreator{ allocation, imageParameters }, memoryAllocator);
}

ErrorOr<Texture> Texture::createImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    const VkImageLayout dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, image, allocation, dstLayout, imageParams, view, sampler, samplerParams);
}

ErrorOr<Texture> Texture::createImageSampler(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkImageLayout dstLayout, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, image, allocation, dstLayout, imageParams, view, sampler, samplerParams);
}

ErrorOr<Texture> Texture::createMipmapImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    generateImageMipmaps(commandBuffer, image, imageParams.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.width, imageParams.height, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, image, allocation, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams, view, sampler, samplerParams);
}

TextureBuilder& TextureBuilder::withFormat(VkFormat format) {
    _imageParameters.format = format;
    return *this;
}

TextureBuilder& TextureBuilder::withExtent(uint32_t width, uint32_t height) {
	_imageParameters.width = width;
	_imageParameters.height = height;
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

TextureBuilder& TextureBuilder::withCompareOp(std::optional<VkCompareOp> compareOp) {
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
    const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, _imageLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    return Texture(logicalDevice, image, allocation, _imageLayout, _imageParameters, view);
}

ErrorOr<Texture> TextureBuilder::buildImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::span<const VkBufferImageCopy> copyRegions) const {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
    const VkSampler sampler = logicalDevice.createSampler(_samplerParameters);
    const VkImageLayout dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
    return Texture(logicalDevice, image, allocation, dstLayout, _imageParameters, view, sampler, _samplerParameters);
}

ErrorOr<Texture> TextureBuilder::buildImageSampler(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const {
	const VkSampler sampler = logicalDevice.createSampler(_samplerParameters);
	Allocation allocation;
	ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
	const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
	transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, _imageLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
	return Texture(logicalDevice, image, allocation, _imageLayout, _imageParameters, view, sampler, _samplerParameters);
}

ErrorOr<Texture> TextureBuilder::buildMipmapImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, std::span<const VkBufferImageCopy> copyRegions) const {
	Allocation allocation;
	ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, _imageParameters, logicalDevice.getMemoryAllocator()));
	const VkImageView view = logicalDevice.createImageView(image, _imageParameters);
	const VkSampler sampler = logicalDevice.createSampler(_samplerParameters);
	transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
	copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
	generateImageMipmaps(commandBuffer, image, _imageParameters.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		_imageParameters.width, _imageParameters.height, _imageParameters.mipLevels, _imageParameters.layerCount);
	return Texture(logicalDevice, image, allocation,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _imageParameters, view, sampler, _samplerParameters);
}
