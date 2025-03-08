#include "texture.h"

#include "logical_device/logical_device.h"
#include "memory_objects/buffers.h"
#include "memory_allocator/memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <stdexcept>

Texture::Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view, const VkSampler sampler, const SamplerParameters& samplerParameters)
    : _logicalDevice(logicalDevice), _type(type), _image(image), _allocation(allocation), _imageParameters(imageParameters), _view(view), _sampler(sampler), _samplerParameters(samplerParameters) {

}

std::unique_ptr<Texture> Texture::create(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view, const VkSampler sampler, const SamplerParameters& samplerParameters) {
    return std::unique_ptr<Texture>(new Texture(logicalDevice, type, image, allocation, imageParameters, view, sampler, samplerParameters));
}

namespace {

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

Texture::~Texture() {
    const VkDevice device = _logicalDevice.getVkDevice();
    if(_sampler)
        vkDestroySampler(device, _sampler, nullptr);
    if(_view)
        vkDestroyImageView(device, _view, nullptr);
    if(_image)
        std::visit(ImageDeleter{ _image }, _logicalDevice.getMemoryAllocator(), _allocation);
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
