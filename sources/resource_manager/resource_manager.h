#pragma once

#include "logical_device/logical_device.h"
#include "memory_objects/buffer.h"

#include <unordered_map>
#include <string_view>

class ResourceManager {
public:
	ResourceManager(const LogicalDevice& logicalDevice);
	Texture* getTexture(std::string_view filePath);
	Texture& create2DTexture(std::string_view filePath, const VkCommandBuffer commandBuffer, VkFormat format, float samplerAnisotropy, const Buffer& stagingBuffer, const ImageDimensions& imageDimensions);
	Texture& createCubemap(std::string_view filePath, const VkCommandBuffer commandBuffer, VkFormat format, float samplerAnisotropy, const Buffer& stagingBuffer, const ImageDimensions& imageDimensions);
	Texture& create2DShadowMap(std::string_view filePath, const VkCommandBuffer commandBuffer, VkFormat format, uint32_t width, uint32_t height);
private:
	const LogicalDevice& _logicalDevice;

	std::unordered_map<std::string, Texture> _textures;	// TODO unordered_node_map
	std::unordered_map<std::string, Buffer> _uniformBuffers; // TODO unordered_node_map
};
