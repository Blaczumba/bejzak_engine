#include "texture.h"

#include "logical_device/logical_device.h"
#include "memory_objects/buffers.h"
#include "memory_objects/staging_buffer.h"
#include "memory_allocator/memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <array>
#include <stdexcept>

Texture::Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view, const VkSampler sampler, const SamplerParameters& samplerParameters)
    : _logicalDevice(logicalDevice), _type(type), _image(image), _allocation(allocation), _imageParameters(imageParameters), _view(view), _sampler(sampler), _samplerParameters(samplerParameters) {

}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format) {
    ImageParameters imageParams = {
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

lib::ErrorOr<std::unique_ptr<Texture>> Texture::create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
    ImageParameters imageParams = {
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
    return createMipmapImage(logicalDevice, commandBuffer, stagingBuffer.getVkBuffer(), dimensions.copyRegions, imageParams, samplerParams);
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
    ImageParameters imageParams = {
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
    return createImage(logicalDevice, commandBuffer, Texture::Type::CUBEMAP, stagingBuffer.getVkBuffer(), dimensions.copyRegions, imageParams, samplerParams);
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
    return createImage(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, Texture::Type::COLOR_ATTACHMENT,
        ImageParameters{
            .format = format,
            .width = extent.width,
            .height = extent.height,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevels = 1,
            .numSamples = samples,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        }
    );
}

namespace {

lib::ErrorOr<VkImage> allocate(Allocation& allocation, const ImageParameters& imageParameters, MemoryAllocator& memoryAllocator) {
    return std::visit(ImageCreator{ allocation, imageParameters }, memoryAllocator);
}

bool hasStencil(VkFormat format) {
    const std::array<VkFormat, 4> formats = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };
    return std::find(formats.begin(), formats.end(), format) != std::end(formats);
}

struct ImageDeleter {
    VkImage image;

    void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
        allocator.destroyVkImage(image, allocation);
    }

    void operator()(auto&&, auto&&) {
        throw std::runtime_error("Invalid memory allocator or memory instance!");
    }
};

}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
    const VkImageAspectFlags aspect = hasStencil(format) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    return createImage(logicalDevice, commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, Texture::Type::DEPTH_ATTACHMENT,
        ImageParameters{
            .format = format,
            .width = extent.width,
            .height = extent.height,
            .aspect = aspect,
            .numSamples = samples,
            .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        }
    );
}

Texture::~Texture() {
    const VkDevice device = _logicalDevice.getVkDevice();
    if (_sampler) {
        vkDestroySampler(device, _sampler, nullptr);
    }
    if (_view) {
        vkDestroyImageView(device, _view, nullptr);
    }
    if(_image) {
        std::visit(ImageDeleter{ _image }, _logicalDevice.getMemoryAllocator(), _allocation);
    }
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

void Texture::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
    transitionImageLayout(commandBuffer, _image, _imageParameters.layout, newLayout, _imageParameters.aspect, _imageParameters.mipLevels, _imageParameters.layerCount);
}


void Texture::generateMipmaps(VkCommandBuffer commandBuffer) {
    const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    generateImageMipmaps(commandBuffer, _image, _imageParameters.format, finalLayout, _imageParameters.width, _imageParameters.height, _imageParameters.mipLevels, _imageParameters.layerCount);
    _imageParameters.layout = finalLayout;
}

const ImageParameters& Texture::getImageParameters() const {
    return _imageParameters;
}

const SamplerParameters& Texture::getSamplerParameters() const {
    return _samplerParameters;
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters&& imageParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = dstLayout;
    return std::unique_ptr<Texture>(new Texture(logicalDevice, type, image, allocation, imageParams, view));
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createImageSampler(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = dstLayout;
    return std::unique_ptr<Texture>(new Texture(logicalDevice, type, image, allocation, imageParams, view, sampler, samplerParams));
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createMipmapImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    generateImageMipmaps(commandBuffer, image, imageParams.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.width, imageParams.height, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return std::unique_ptr<Texture>(new Texture(logicalDevice, Texture::Type::IMAGE_2D, image, allocation, imageParams, view, sampler, samplerParams));
}

lib::ErrorOr<std::unique_ptr<Texture>> Texture::createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, Texture::Type type, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    ASSIGN_OR_RETURN(const VkImage image, allocate(allocation, imageParams, logicalDevice.getMemoryAllocator()));
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return std::unique_ptr<Texture>(new Texture(logicalDevice, type, image, allocation, imageParams, view, sampler, samplerParams));
}
