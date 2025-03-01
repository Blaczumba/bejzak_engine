#pragma once

#include "lib/status/status.h"
#include "memory_objects/texture/texture.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class Window;
class LogicalDevice;
class PhysicalDevice;
class Texture;

class Swapchain {
	VkSwapchainKHR _swapchain;
	const LogicalDevice& _logicalDevice;

	VkSurfaceFormatKHR _surfaceFormat;
	VkExtent2D _extent;
	std::vector<VkImage> _images;
	std::vector<VkImageView> _views;

	Swapchain(const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkSurfaceFormatKHR format, VkExtent2D extent, std::vector<VkImage>&& images, std::vector<VkImageView>&& views);

public:
	// If we want to recreate the swapchain use this factory method and pass an old (currently existing) swapchain.
	static lib::ErrorOr<std::unique_ptr<Swapchain>> create(const LogicalDevice& logicalDevice, const Swapchain* oldSwapchain = nullptr);

	~Swapchain();

	const VkSwapchainKHR getVkSwapchain() const;
	const VkFormat getVkFormat() const;
	VkExtent2D getExtent() const;
	uint32_t getImagesCount() const;
	const std::vector<VkImage>& getVkImages() const;
	const std::vector<VkImageView>& getVkImageViews() const;

	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const;
	VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore) const;
};