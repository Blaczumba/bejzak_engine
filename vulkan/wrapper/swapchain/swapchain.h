#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class Swapchain {
  Swapchain(VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkFormat format,
            VkExtent2D extent, std::vector<VkImage>&& images,
            std::vector<VkImageView>&& views) noexcept;

public:
  Swapchain() = default;

  Swapchain(Swapchain&& other) noexcept;

  Swapchain& operator=(Swapchain&& other) noexcept;

  ~Swapchain();

  const VkSwapchainKHR getVkSwapchain() const noexcept;

  const VkFormat getVkFormat() const noexcept;

  VkExtent2D getExtent() const noexcept;

  uint32_t getImagesCount() const noexcept;

  std::span<const VkImageView> getImageViews() const noexcept;

  const VkImageView getSwapchainVkImageView(size_t index) const noexcept;

  VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const;

  VkResult present(uint32_t imageIndex, VkSemaphore waitSemaphore) const;

  const LogicalDevice& getLogicalDevice() const noexcept;

private:
  void destroy();

  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
  const LogicalDevice* _logicalDevice = nullptr;

  VkFormat _surfaceFormat;
  VkExtent2D _extent;
  std::vector<VkImage> _images;
  std::vector<VkImageView> _views;

  friend class SwapchainBuilder;
};

class SwapchainBuilder {
public:
  SwapchainBuilder& withOldSwapchain(VkSwapchainKHR oldSwapchain) noexcept;

  SwapchainBuilder& withPreferredFormat(VkFormat format) noexcept;

  SwapchainBuilder& withPreferredPresentMode(VkPresentModeKHR presentMode) noexcept;

  SwapchainBuilder& withImageArrayLayers(uint32_t layers) noexcept;

  SwapchainBuilder& withCompositeAlpha(VkCompositeAlphaFlagBitsKHR compositeAlpha) noexcept;

  SwapchainBuilder& withClipped(VkBool32 clipped) noexcept;

  Swapchain build(const LogicalDevice& logicalDevice, VkSurfaceKHR surface, VkExtent2D extent);

private:
  VkSwapchainKHR _oldSwapchain = nullptr;
  VkFormat _preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkPresentModeKHR _preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t imageArrayLayers = 1;
  VkCompositeAlphaFlagBitsKHR _compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  VkBool32 _clipped = VK_TRUE;
};
