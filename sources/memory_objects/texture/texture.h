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

	static std::unique_ptr<Texture> create(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view = VK_NULL_HANDLE, const VkSampler sampler = VK_NULL_HANDLE, const SamplerParameters& samplerParameters = {});

	void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout);

	const VkImage getVkImage() const;
	const VkImageView getVkImageView() const;
	const VkSampler getVkSampler() const;
	const ImageParameters& getImageParameters() const;
	const SamplerParameters& getSamplerParameters() const;
	VkExtent2D getVkExtent2D() const;

private:
	Texture(const LogicalDevice& logicalDevice, Texture::Type type, const VkImage image, const Allocation allocation, const ImageParameters& imageParameters, const VkImageView view, const VkSampler sampler, const SamplerParameters& samplerParameters);
	
	const Type _type;

	Allocation _allocation;
	VkImage _image;
	VkImageView _view;
	VkSampler _sampler;

	ImageParameters _imageParameters;
	SamplerParameters _samplerParameters;

	const LogicalDevice& _logicalDevice;

	void generateMipmaps(VkCommandBuffer commandBuffer);
};

namespace {

struct ImageCreator {
	Allocation& allocation;
	const ImageParameters& params;

	const lib::ErrorOr<VkImage> operator()(VmaWrapper& allocator) {
		ASSIGN_OR_RETURN(std::pair imageData, allocator.createVkImage(params, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
		allocation = imageData.second;
		return imageData.first;
	}

	const lib::ErrorOr<VkImage> operator()(auto&&) {
		return lib::Error("Unrecognized allocator during Texture creation");
	}
};

}
