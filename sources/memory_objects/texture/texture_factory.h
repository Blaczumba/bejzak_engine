#pragma once

#include "texture.h"
#include "memory_objects/staging_buffer.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <string_view>

class CommandPool;

class TextureFactory {
public:
	static std::unique_ptr<Texture> createTextureCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	static std::unique_ptr<Texture> create2DTextureImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	static std::unique_ptr<Texture> create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format);
	static std::unique_ptr<Texture> createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);
	static std::unique_ptr<Texture> createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);
};

