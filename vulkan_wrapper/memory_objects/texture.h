#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"
#include "vulkan_wrapper/memory_allocator/allocation.h"
#include "vulkan_wrapper/memory_objects/image.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <span>
#include <variant>

class LogicalDevice;

struct Texture {
public:
	Texture();

	Texture(Texture&& texture) noexcept;

	Texture& operator=(Texture&& texuture) noexcept;

	~Texture();
	
	static ErrorOr<Texture> create2DImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);
	
	static ErrorOr<Texture> create2DShadowmap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format);
	
	static ErrorOr<Texture> createCubemap(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer stagingBuffer, const ImageDimensions& dimensions, VkFormat format, float samplerAnisotropy);

	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

	VkImage getVkImage() const;

	VkImageView getVkImageView() const;

	VkSampler getVkSampler() const;

	const ImageParameters& getImageParameters() const;

	const SamplerParameters& getSamplerParameters() const;

	VkExtent2D getVkExtent2D() const;

	VkImageLayout getVkImageLayout() const;

private:
	Texture(const LogicalDevice& logicalDevice, VkImage image, const Allocation allocation, VkImageLayout layout, const ImageParameters& imageParameters, VkImageView view = VK_NULL_HANDLE, VkSampler sampler = VK_NULL_HANDLE, const SamplerParameters& samplerParameters = {});

	Allocation _allocation;
	VkImage _image;
	VkImageView _view;
	VkSampler _sampler;
	VkImageLayout _layout;

	ImageParameters _imageParameters;
	SamplerParameters _samplerParameters;

	const LogicalDevice* _logicalDevice;

	friend class TextureBuilder;

	static ErrorOr<Texture> createImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static ErrorOr<Texture> createImageSampler(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkImageLayout dstLayout, const ImageParameters& imageParams, const SamplerParameters& samplerParams);

	static ErrorOr<Texture> createMipmapImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::vector<VkBufferImageCopy>& copyRegions, const ImageParameters& imageParams, const SamplerParameters& samplerParams);
};

class TextureBuilder {
public:
	TextureBuilder& withFormat(VkFormat format);

	TextureBuilder& withExtent(uint32_t width, uint32_t height);

	TextureBuilder& withAspect(VkImageAspectFlags aspect);

	TextureBuilder& withMipLevels(uint32_t mipLevels);

	TextureBuilder& withNumSamples(VkSampleCountFlagBits numSamples);

	TextureBuilder& withTiling(VkImageTiling tiling);

	TextureBuilder& withUsage(VkImageUsageFlags usage);

	TextureBuilder& withProperties(VkMemoryPropertyFlags properties);

	TextureBuilder& withLayerCount(uint32_t layerCount);

	TextureBuilder& withMagFilter(VkFilter magFilter);

	TextureBuilder& withMinFilter(VkFilter minFilter);

	TextureBuilder& withMipmapMode(VkSamplerMipmapMode mipmapMode);

	TextureBuilder& withAddressModes(VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW);

	TextureBuilder& withMipLodBias(float mipLodBias);

	TextureBuilder& withMaxAnisotropy(float maxAnisotropy);

	TextureBuilder& withCompareOp(std::optional<VkCompareOp> compareOp);

	TextureBuilder& withMinLod(float minLod);

	TextureBuilder& withMaxLod(float maxLod);

	TextureBuilder& withBorderColor(VkBorderColor borderColor);

	TextureBuilder& withUnnormalizedCoordinates(VkBool32 unnormalizedCoordinates);

	ErrorOr<Texture> buildAttachment(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const;

	ErrorOr<Texture> buildImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, const std::span<const VkBufferImageCopy> copyRegions) const;

	ErrorOr<Texture> buildImageSampler(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer) const;

	ErrorOr<Texture> buildMipmapImage(const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkBuffer copyBuffer, std::span<const VkBufferImageCopy> copyRegions) const;

private:
	ImageParameters _imageParameters;
	SamplerParameters _samplerParameters;
	VkImageLayout _imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};
