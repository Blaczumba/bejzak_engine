#pragma once

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/logical_device/logical_device.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

class Swapchain {
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
	const LogicalDevice* _logicalDevice = nullptr;

	VkSurfaceFormatKHR _surfaceFormat;
	VkExtent2D _extent;
	lib::Buffer<VkImage> _images;
	lib::Buffer<VkImageView> _views;

public:
	Swapchain() = default;

	Swapchain(VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkSurfaceFormatKHR format, VkExtent2D extent, uint32_t imageCount);

	Swapchain(Swapchain&& other) noexcept;

	Swapchain& operator=(Swapchain&& other) noexcept;

	~Swapchain();

	const VkSwapchainKHR getVkSwapchain() const;

	const VkFormat getVkFormat() const;

	VkExtent2D getExtent() const;

	uint32_t getImagesCount() const;

	const VkImageView getSwapchainVkImageView(size_t index) const;

	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const;

	VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore) const;
};

class SwapchainBuilder {
	VkSwapchainKHR _oldSwapchain = nullptr;
	VkFormat _preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
	VkPresentModeKHR _preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t imageArrayLayers = 1;
	VkCompositeAlphaFlagBitsKHR _compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkBool32 _clipped = VK_TRUE;

public:
	SwapchainBuilder& withOldSwapchain(VkSwapchainKHR oldSwapchain);
	SwapchainBuilder& withPreferredFormat(VkFormat format);
	SwapchainBuilder& withPreferredPresentMode(VkPresentModeKHR presentMode);
	SwapchainBuilder& withImageArrayLayers(uint32_t layers);
	SwapchainBuilder& withCompositeAlpha(VkCompositeAlphaFlagBitsKHR compositeAlpha);
	SwapchainBuilder& withClipped(VkBool32 clipped);
	ErrorOr<Swapchain> build(const LogicalDevice& logicalDevice, VkExtent2D extent);
};
