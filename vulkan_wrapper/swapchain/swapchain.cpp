#include "swapchain.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <span>

#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/physical_device/physical_device.h"
#include "vulkan_wrapper/util/check.h"

Swapchain::Swapchain(
    const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkFormat surfaceFormat,
    VkExtent2D extent, lib::Buffer<VkImage>&& images, lib::Buffer<VkImageView>&& views)
  : _swapchain(swapchain), _logicalDevice(&logicalDevice), _surfaceFormat(surfaceFormat),
    _extent(extent), _images(std::move(images)), _views(std::move(views)) {}

Swapchain::Swapchain(Swapchain&& swapchain) noexcept
  : _swapchain(std::exchange(swapchain._swapchain, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(swapchain._logicalDevice, nullptr)),
    _surfaceFormat(swapchain._surfaceFormat), _extent(swapchain._extent),
    _images(std::move(swapchain._images)), _views(std::move(swapchain._views)) {}

void Swapchain::tryDestroySwapchain() {
  for (const VkImageView view : _views) {
    vkDestroyImageView(_logicalDevice->getVkDevice(), view, nullptr);
  }

  if (_swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(_logicalDevice->getVkDevice(), _swapchain, nullptr);
  }
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  tryDestroySwapchain();

  _swapchain = std::exchange(other._swapchain, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(other._logicalDevice, nullptr);
  _surfaceFormat = other._surfaceFormat;
  _extent = other._extent;
  _images = std::move(other._images);
  _views = std::move(other._views);
  return *this;
}

Swapchain::~Swapchain() {
  tryDestroySwapchain();
}

const VkSwapchainKHR Swapchain::getVkSwapchain() const {
  return _swapchain;
}

VkExtent2D Swapchain::getExtent() const {
  return _extent;
}

const VkFormat Swapchain::getVkFormat() const {
  return _surfaceFormat;
}

uint32_t Swapchain::getImagesCount() const {
  return _images.size();
}

const VkImageView Swapchain::getSwapchainVkImageView(size_t index) const {
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

SwapchainBuilder& SwapchainBuilder::withOldSwapchain(VkSwapchainKHR oldSwapchain) {
  _oldSwapchain = oldSwapchain;
  return *this;
}
SwapchainBuilder& SwapchainBuilder::withPreferredFormat(VkFormat format) {
  _preferredFormat = format;
  return *this;
}
SwapchainBuilder& SwapchainBuilder::withPreferredPresentMode(VkPresentModeKHR presentMode) {
  _preferredPresentMode = presentMode;
  return *this;
}
SwapchainBuilder& SwapchainBuilder::withImageArrayLayers(uint32_t layers) {
  imageArrayLayers = layers;
  return *this;
}
SwapchainBuilder& SwapchainBuilder::withCompositeAlpha(VkCompositeAlphaFlagBitsKHR compositeAlpha) {
  _compositeAlpha = compositeAlpha;
  return *this;
}
SwapchainBuilder& SwapchainBuilder::withClipped(VkBool32 clipped) {
  _clipped = clipped;
  return *this;
}

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    std::span<const VkSurfaceFormatKHR> availableFormats, VkFormat preferredFormat) {
  auto availableFormat = std::find_if(
      std::cbegin(availableFormats), std::cend(availableFormats), [=](const auto& format) {
        return format.format == preferredFormat
               && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      });

  return (availableFormat != std::cend(availableFormats)) ? *availableFormat : availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    std::span<const VkPresentModeKHR> availablePresentModes, VkPresentModeKHR preferredMode) {
  auto availablePresentMode = std::find(
      std::cbegin(availablePresentModes), std::cend(availablePresentModes), preferredMode);

  return (availablePresentMode != std::cend(availablePresentModes)) ?
             *availablePresentMode :
             VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
    VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  return {std::clamp(actualWindowExtent.width, capabilities.minImageExtent.width,
                     capabilities.maxImageExtent.width),
          std::clamp(actualWindowExtent.height, capabilities.minImageExtent.height,
                     capabilities.maxImageExtent.height)};
}

}  // namespace

ErrorOr<Swapchain> SwapchainBuilder::build(
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
  CHECK_VKCMD(vkCreateSwapchainKHR(logicalDevice.getVkDevice(), &createInfo, nullptr, &swapchain));

  vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, nullptr);
  lib::Buffer<VkImage> images(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, images.data());

  lib::Buffer<VkImageView> views(imageCount);
  for (size_t i = 0; i < images.size(); ++i) {
    ASSIGN_OR_RETURN(views[i], logicalDevice.createImageView(
                                   images[i], VK_IMAGE_VIEW_TYPE_2D, surfaceFormat.format,
                                   VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1));
  }

  return Swapchain(swapchain, logicalDevice, surfaceFormat.format, actualExtent, std::move(images),
                   std::move(views));
}
