#include "texture.h"

#include "logical_device/logical_device.h"
#include "memory_objects/buffer.h"
#include "memory_objects/buffers.h"
#include "memory_allocator/memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <array>

Texture::Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation,
    VkImageLayout layout, const ImageParameters& imageParameters, const VkImageView view, const VkSampler sampler,
    const SamplerParameters& samplerParameters)
    : _logicalDevice(&logicalDevice), _type(type), _image(image), _allocation(allocation), _layout(layout),
    _imageParameters(imageParameters), _view(view), _sampler(sampler), _samplerParameters(samplerParameters) {

}

Texture::Texture() : _image(VK_NULL_HANDLE), _view(VK_NULL_HANDLE), _sampler(VK_NULL_HANDLE) {

}

Texture::Texture(Texture&& texture) noexcept
    : _type(texture._type), _allocation(texture._allocation), _image(std::exchange(texture._image, VK_NULL_HANDLE)), 
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
    return createImageSampler(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Texture::Type::SHADOWMAP, imageParams, samplerParams);
}

ErrorOr<Texture> Texture::create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
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

ErrorOr<Texture> Texture::createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
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
    return createImage(logicalDevice, commandBuffer, Texture::Type::CUBEMAP, stagingBuffer, dimensions.copyRegions, imageParams, samplerParams);
}

ErrorOr<Texture> Texture::createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
    const ImageParameters imageParams = {
        .format = format,
        .width = extent.width,
        .height = extent.height,
        .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevels = 1,
        .numSamples = samples,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    return createAttachment(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, Texture::Type::COLOR_ATTACHMENT, imageParams);
}

static bool hasStencil(VkFormat format) {
    static constexpr std::array<VkFormat, 4> formats = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };
    return std::find(formats.cbegin(), formats.cend(), format) != std::cend(formats);
}

ErrorOr<Texture> Texture::createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
    const VkImageAspectFlags aspect = hasStencil(format) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    const ImageParameters imageParams = {
        .format = format,
        .width = extent.width,
        .height = extent.height,
        .aspect = aspect,
        .numSamples = samples,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    };
    return createAttachment(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, Texture::Type::DEPTH_ATTACHMENT, imageParams);
}

const VkImage Texture::getVkImage() const {
    return _image;
}

const VkImageView Texture::getVkImageView() const {
    return _view;
}

const VkSampler Texture::getVkSampler() const {
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

ErrorOr<Texture> Texture::createAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, const ImageParameters& imageParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, type, image, allocation, dstLayout, imageParams, view);
}

ErrorOr<Texture> Texture::createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, Texture::Type type, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    const VkImageLayout dstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, type, image, allocation, dstLayout, imageParams, view, sampler, samplerParams);
}

ErrorOr<Texture> Texture::createImageSampler(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, type, image, allocation, dstLayout, imageParams, view, sampler, samplerParams);
}

ErrorOr<Texture> Texture::createMipmapImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    generateImageMipmaps(commandBuffer, image, imageParams.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.width, imageParams.height, imageParams.mipLevels, imageParams.layerCount);
    return Texture(logicalDevice, Texture::Type::IMAGE_2D, image, allocation, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams, view, sampler, samplerParams);
}
