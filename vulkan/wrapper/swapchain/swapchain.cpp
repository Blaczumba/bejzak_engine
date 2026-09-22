#include "swapchain.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/util/check.h"

Swapchain::Swapchain(
    const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkFormat surfaceFormat,
    VkExtent2D extent, std::vector<VkImage>&& images, std::vector<VkImageView>&& views) noexcept
  : _swapchain(swapchain), _logicalDevice(&logicalDevice), _surfaceFormat(surfaceFormat),
    _extent(extent), _images(std::move(images)), _views(std::move(views)) {}

Swapchain::Swapchain(Swapchain&& swapchain) noexcept
  : _swapchain(std::exchange(swapchain._swapchain, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(swapchain._logicalDevice, nullptr)),
    _surfaceFormat(swapchain._surfaceFormat), _extent(swapchain._extent),
    _images(std::move(swapchain._images)), _views(std::move(swapchain._views)) {}

void Swapchain::destroy() {
  for (const VkImageView view : _views) {
    _logicalDevice->destroyResource([view](DestroyerContext context) {
      vkDestroyImageView(context.device, view, context.allocationCallbacks);
    });
  }
  _views.clear();

  if (_swapchain != VK_NULL_HANDLE) {
    _logicalDevice->destroyResource([swapchain = _swapchain](DestroyerContext context) {
      vkDestroySwapchainKHR(context.device, swapchain, context.allocationCallbacks);
    });
    _swapchain = VK_NULL_HANDLE;
  }
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  destroy();

  _swapchain = std::exchange(other._swapchain, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(other._logicalDevice, nullptr);
  _surfaceFormat = other._surfaceFormat;
  _extent = other._extent;
  _images = std::move(other._images);
  _views = std::move(other._views);
  return *this;
}

Swapchain::~Swapchain() {
  destroy();
}

const VkSwapchainKHR Swapchain::getVkSwapchain() const noexcept {
  return _swapchain;
}

VkExtent2D Swapchain::getExtent() const noexcept {
  return _extent;
}

const VkFormat Swapchain::getVkFormat() const noexcept {
  return _surfaceFormat;
}

uint32_t Swapchain::getImagesCount() const noexcept {
  return _images.size();
}

std::span<const VkImageView> Swapchain::getImageViews() const noexcept {
  return _views;
}

const VkImageView Swapchain::getSwapchainVkImageView(size_t index) const noexcept {
  if (index < _views.size()) {
    return _views[index];
  }

  return VK_NULL_HANDLE;
}

VkResult Swapchain::acquireNextImage(
    VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const {
  return vkAcquireNextImageKHR(_logicalDevice->getVkDevice(), _swapchain, UINT64_MAX,
                               presentCompleteSemaphore, nullptr, imageIndex);
}

VkResult Swapchain::present(uint32_t imageIndex, VkSemaphore waitSemaphore) const {
  const VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &waitSemaphore,
    .swapchainCount = 1,
    .pSwapchains = &_swapchain,
    .pImageIndices = &imageIndex,
  };

  return vkQueuePresentKHR(_logicalDevice->getPresentVkQueue(), &presentInfo);
}

const LogicalDevice& Swapchain::getLogicalDevice() const noexcept {
  return *_logicalDevice;
}

SwapchainBuilder& SwapchainBuilder::withOldSwapchain(VkSwapchainKHR oldSwapchain) noexcept {
  _oldSwapchain = oldSwapchain;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withPreferredFormat(VkFormat format) noexcept {
  _preferredFormat = format;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withPreferredPresentMode(
    VkPresentModeKHR presentMode) noexcept {
  _preferredPresentMode = presentMode;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withImageArrayLayers(uint32_t layers) noexcept {
  imageArrayLayers = layers;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withCompositeAlpha(
    VkCompositeAlphaFlagBitsKHR compositeAlpha) noexcept {
  _compositeAlpha = compositeAlpha;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withClipped(VkBool32 clipped) noexcept {
  _clipped = clipped;
  return *this;
}

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    std::span<const VkSurfaceFormatKHR> availableFormats, VkFormat preferredFormat) noexcept {
  auto availableFormat = std::find_if(
      std::cbegin(availableFormats), std::cend(availableFormats), [=](const auto& format) {
        return format.format == preferredFormat
               && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      });

  return (availableFormat != std::cend(availableFormats)) ? *availableFormat : availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(std::span<const VkPresentModeKHR> availablePresentModes,
                                       VkPresentModeKHR preferredMode) noexcept {
  auto availablePresentMode = std::find(
      std::cbegin(availablePresentModes), std::cend(availablePresentModes), preferredMode);

  return (availablePresentMode != std::cend(availablePresentModes)) ?
             *availablePresentMode :
             VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
    VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities) noexcept {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  return {std::clamp(actualWindowExtent.width, capabilities.minImageExtent.width,
                     capabilities.maxImageExtent.width),
          std::clamp(actualWindowExtent.height, capabilities.minImageExtent.height,
                     capabilities.maxImageExtent.height)};
}

}  // namespace

Swapchain SwapchainBuilder::build(
    const LogicalDevice& logicalDevice, VkSurfaceKHR surface, VkExtent2D extent) {
  const SwapChainSupportDetails swapChainSupport =
      logicalDevice.getPhysicalDevice().getSwapchainSupportDetails(surface);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0
      && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  const VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats, _preferredFormat);

  const VkExtent2D actualExtent = chooseSwapExtent(extent, swapChainSupport.capabilities);
  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = actualExtent,
    .imageArrayLayers = imageArrayLayers,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = swapChainSupport.capabilities.currentTransform,
    .compositeAlpha = _compositeAlpha,
    .presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, _preferredPresentMode),
    .clipped = _clipped,
    .oldSwapchain = _oldSwapchain};

  const QueueFamilyIndices indices = logicalDevice.getPhysicalDevice().getQueueFamilyIndices();

  if (indices.graphicsFamily != indices.presentFamily) {
    const uint32_t queueFamilyIndices[] = {
      indices.graphicsFamily.value(), indices.presentFamily.value()};

    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = static_cast<uint32_t>(std::size(queueFamilyIndices));
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  VkSwapchainKHR swapchain;
  CHECK_VKCMD(vkCreateSwapchainKHR(logicalDevice.getVkDevice(), &createInfo, nullptr, &swapchain),
              "Failed to create VkSwapchainKHR.");

  vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, nullptr);
  std::vector<VkImage> images(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, images.data());

  std::vector<VkImageView> views(imageCount);
  std::transform(
      images.cbegin(), images.cend(), views.begin(),
      [logicalDevice = &logicalDevice, format = surfaceFormat.format](const VkImage image) {
        const VkImageViewCreateInfo imageViewCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = image,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = format,
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                               .baseMipLevel = 0,
                               .levelCount = 1,
                               .baseArrayLayer = 0,
                               .layerCount = 1}
        };

        return logicalDevice->createImageView(imageViewCreateInfo);
      });

  return Swapchain(swapchain, logicalDevice, surfaceFormat.format, actualExtent, std::move(images),
                   std::move(views));
}
