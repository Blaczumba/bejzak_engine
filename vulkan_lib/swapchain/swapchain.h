#pragma once

#include "vulkan_lib/lib/buffer/buffer.h"
#include "vulkan_lib/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class LogicalDevice;

class Swapchain {
	VkSwapchainKHR _swapchain;
	const LogicalDevice& _logicalDevice;

	VkSurfaceFormatKHR _surfaceFormat;
	VkExtent2D _extent;
	lib::Buffer<VkImage> _images;
	lib::Buffer<VkImageView> _views;

	Swapchain(const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkSurfaceFormatKHR format, VkExtent2D extent, uint32_t imageCount);

public:
	// If we want to recreate the swapchain use this factory method and pass an old (currently existing) swapchain.
	static ErrorOr<std::unique_ptr<Swapchain>> create(const LogicalDevice& logicalDevice, VkSwapchainKHR oldSwapchain = nullptr);

	~Swapchain();

	const VkSwapchainKHR getVkSwapchain() const;

	const VkFormat getVkFormat() const;

	VkExtent2D getExtent() const;

	uint32_t getImagesCount() const;

	const VkImageView getSwapchainVkImageView(size_t index) const;

	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const;

	VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore) const;
};