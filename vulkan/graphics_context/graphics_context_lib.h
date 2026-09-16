#pragma once

#include <span>
#include <utility>
#include <vulkan/vulkan.h>

#include "common/buffer/index_buffer_lib.h"
#include "common/model_loader/image_loader/types.h"
#include "lib/buffer/buffer.h"

namespace vlkn::internal {

// Framebuffer createFramebufferFromTextures(
//     const Renderpass& renderpass, std::span<const Image> textures);

lib::Buffer<VkDescriptorPoolSize> getDescriptorPoolSizesFromBindings(
    std::span<const std::pair<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> bindings);

VkIndexType convertIndexTypeToVkIndexType(common::IndexType indexType);

lib::Buffer<VkBufferImageCopy> translateImageSubresourcesToVkBufferImageCopy(
    std::span<const ImageSubresource> imageSubresources, size_t stagingBufferOffset = 0);

}  // namespace vlkn::internal
