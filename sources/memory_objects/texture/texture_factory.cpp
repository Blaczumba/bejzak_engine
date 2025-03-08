#include "texture_factory.h"

#include "logical_device/logical_device.h"
#include "memory_objects/staging_buffer.h"
#include "model_loader/image_loader/image_loader.h"

#include <array>

namespace {

const VkImage allocate(Allocation& allocation, const ImageParameters& imageParameters, MemoryAllocator& memoryAllocator) {
    return std::visit(ImageCreator{ allocation, imageParameters }, memoryAllocator);
}

std::unique_ptr<Texture> createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters&& imageParams) {
    Allocation allocation;
    const VkImage image = allocate(allocation, imageParams, logicalDevice.getMemoryAllocator());
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = dstLayout;
    return Texture::create(logicalDevice, type, image, allocation, imageParams, view);
}

std::unique_ptr<Texture> createImageSampler(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    Allocation allocation;
    const VkImage image = allocate(allocation, imageParams, logicalDevice.getMemoryAllocator());
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, dstLayout, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = dstLayout;
    return Texture::create(logicalDevice, type, image, allocation, imageParams, view, sampler, samplerParams);
}

std::unique_ptr<Texture> createTextureMipmapImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    const VkImage image = allocate(allocation, imageParams, logicalDevice.getMemoryAllocator());
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    generateImageMipmaps(commandBuffer, image, imageParams.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.width, imageParams.height, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return Texture::create(logicalDevice, Texture::Type::IMAGE_2D, image, allocation, imageParams, view, sampler, samplerParams);
}

std::unique_ptr<Texture> createTextureImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, Texture::Type type, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams) {
    Allocation allocation;
    const VkImage image = allocate(allocation, imageParams, logicalDevice.getMemoryAllocator());
    const VkImageView view = logicalDevice.createImageView(image, imageParams);
    const VkSampler sampler = logicalDevice.createSampler(samplerParams);
    transitionImageLayout(commandBuffer, image, imageParams.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    copyBufferToImage(commandBuffer, copyBuffer, image, copyRegions);
    transitionImageLayout(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, imageParams.aspect, imageParams.mipLevels, imageParams.layerCount);
    imageParams.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return Texture::create(logicalDevice, type, image, allocation, imageParams, view, sampler, samplerParams);
}

}

std::unique_ptr<Texture> TextureFactory::create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format) {
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

std::unique_ptr<Texture> TextureFactory::create2DTextureImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
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
    return createTextureMipmapImage(logicalDevice, commandBuffer, stagingBuffer.getVkBuffer(), dimensions.copyRegions, imageParams, samplerParams);
}

std::unique_ptr<Texture> TextureFactory::createTextureCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy) {
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
    return createTextureImage(logicalDevice, commandBuffer, Texture::Type::CUBEMAP, stagingBuffer.getVkBuffer(), dimensions.copyRegions, imageParams, samplerParams);
}

std::unique_ptr<Texture> TextureFactory::createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
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

bool hasStencil(VkFormat format) {
    const std::array<VkFormat, 4> formats = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };
    return std::find(formats.begin(), formats.end(), format) != std::end(formats);
}

std::unique_ptr<Texture> TextureFactory::createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent) {
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
