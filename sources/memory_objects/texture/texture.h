#pragma once

#include "lib/status/status.h"
#include "memory_allocator/memory_allocator.h"
#include "memory_allocator/allocation.h"
#include "memory_objects/buffers.h"
#include "memory_objects/texture/image.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>

class LogicalDevice;
class Buffer;

struct Texture {
public:
	enum class Type : uint8_t {
		IMAGE_2D,
		SHADOWMAP,
		COLOR_ATTACHMENT,
		DEPTH_ATTACHMENT,
		CUBEMAP
	};

	~Texture();
	
	static lib::ErrorOr<std::unique_ptr<Texture>> create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const Buffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	
	static lib::ErrorOr<std::unique_ptr<Texture>> create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format);
	
	static lib::ErrorOr<std::unique_ptr<Texture>> createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const Buffer& stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	
	static lib::ErrorOr<std::unique_ptr<Texture>> createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);
	
	static lib::ErrorOr<std::unique_ptr<Texture>> createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);

	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

	const VkImage getVkImage() const;

	const VkImageView getVkImageView() const;

	const VkSampler getVkSampler() const;

	const ImageParameters& getImageParameters() const;

	const SamplerParameters& getSamplerParameters() const;

	VkExtent2D getVkExtent2D() const;

private:
	Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view = VK_NULL_HANDLE, const VkSampler sampler = VK_NULL_HANDLE, const SamplerParameters& samplerParameters = {});
	
	Type _type;

	Allocation _allocation;
	VkImage _image;
	VkImageView _view;
	VkSampler _sampler;

	ImageParameters _imageParameters;
	SamplerParameters _samplerParameters;

	const LogicalDevice& _logicalDevice;

	void generateMipmaps(VkCommandBuffer commandBuffer);

	// Helper functions.
	static lib::ErrorOr<std::unique_ptr<Texture>> createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters&& imageParams);

	static lib::ErrorOr<std::unique_ptr<Texture>> createImageSampler(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static lib::ErrorOr<std::unique_ptr<Texture>> createMipmapImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static lib::ErrorOr<std::unique_ptr<Texture>> createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, Texture::Type type, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, ImageParameters& imageParams, const SamplerParameters& samplerParams);
};

namespace {

struct ImageCreator {
	Allocation& allocation;
	const ImageParameters& params;

	const lib::ErrorOr<VkImage> operator()(VmaWrapper& allocator) {
		ASSIGN_OR_RETURN(VmaWrapper::Image imageData, allocator.createVkImage(params, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
		allocation = imageData.allocation;
		return imageData.image;
	}

	const lib::ErrorOr<VkImage> operator()(auto&&) {
		return lib::Error("Unrecognized allocator during Texture creation");
	}
};

}
