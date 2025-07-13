#pragma once

#include "lib/macros/status_macros.h"
#include "memory_allocator/memory_allocator.h"
#include "memory_allocator/allocation.h"
#include "memory_objects/image.h"
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <variant>

class LogicalDevice;

struct Texture {
public:
	enum class Type : uint8_t {
		IMAGE_2D,
		SHADOWMAP,
		COLOR_ATTACHMENT,
		DEPTH_ATTACHMENT,
		CUBEMAP
	};

	Texture();

	Texture(Texture&& texture) noexcept;

	Texture& operator=(Texture&& texuture) noexcept;

	~Texture();
	
	static ErrorOr<Texture> create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	
	static ErrorOr<Texture> create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format);
	
	static ErrorOr<Texture> createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, const VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	
	static ErrorOr<Texture> createColorAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);
	
	static ErrorOr<Texture> createDepthAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkFormat format, VkSampleCountFlagBits samples, VkExtent2D extent);

	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

	VkImage getVkImage() const;

	VkImageView getVkImageView() const;

	VkSampler getVkSampler() const;

	const ImageParameters& getImageParameters() const;

	const SamplerParameters& getSamplerParameters() const;

	VkExtent2D getVkExtent2D() const;

	VkImageLayout getVkImageLayout() const;

private:
	Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, VkImageLayout layout, const ImageParameters& imageParameters, const VkImageView view = VK_NULL_HANDLE, const VkSampler sampler = VK_NULL_HANDLE, const SamplerParameters& samplerParameters = {});
	
	Type _type;

	Allocation _allocation;
	VkImage _image;
	VkImageView _view;
	VkSampler _sampler;
	VkImageLayout _layout;

	ImageParameters _imageParameters;
	SamplerParameters _samplerParameters;

	const LogicalDevice* _logicalDevice;

	// Helper functions.
	static ErrorOr<Texture> createAttachment(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, const ImageParameters& imageParams);

	static ErrorOr<Texture> createImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, Texture::Type type, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static ErrorOr<Texture> createImageSampler(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, VkImageLayout dstLayout, Texture::Type type, const ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static ErrorOr<Texture> createMipmapImage(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams);
};
