#include "swapchain.h"

#include "vulkan_lib/logical_device/logical_device.h"
#include "vulkan_lib/physical_device/physical_device.h"

#include <algorithm>
#include <limits>
#include <span>

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::span<const VkSurfaceFormatKHR> availableFormats, VkFormat preferredFormat);
VkPresentModeKHR chooseSwapPresentMode(std::span<const VkPresentModeKHR> availablePresentModes, VkPresentModeKHR preferredMode);
VkExtent2D chooseSwapExtent(VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities);

}   // namespace

Swapchain::Swapchain(const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkSurfaceFormatKHR surfaceFormat, VkExtent2D extent, uint32_t imageCount)
	: _swapchain(swapchain), _logicalDevice(logicalDevice), _surfaceFormat(surfaceFormat), _extent(extent), _images(imageCount), _views(imageCount) {
    vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, _images.data());
    std::transform(_images.cbegin(), _images.cend(), _views.begin(),
        [&](const VkImage image) {
        return logicalDevice.createImageView(image, ImageParameters{
                .format = surfaceFormat.format,
                .width = extent.width,
                .height = extent.height,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            });
        });
}

Swapchain::~Swapchain() {
    const VkDevice device = _logicalDevice.getVkDevice();
    for (const VkImageView view : _views) {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, _swapchain, nullptr);
}

const VkSwapchainKHR Swapchain::getVkSwapchain() const {
    return _swapchain;
}

VkExtent2D Swapchain::getExtent() const {
    return _extent;
}

const VkFormat Swapchain::getVkFormat() const {
    return _surfaceFormat.format;
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

ErrorOr<std::unique_ptr<Swapchain>> Swapchain::create(const LogicalDevice& logicalDevice, VkSwapchainKHR oldSwapchain) {
    const SwapChainSupportDetails swapChainSupport = logicalDevice.getPhysicalDevice().getSwapchainSupportDetails();
    const VkDevice device = logicalDevice.getVkDevice();
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, VK_PRESENT_MODE_MAILBOX_KHR);
    const Surface& surface = logicalDevice.getPhysicalDevice().getSurface();
    const Window& window = surface.getWindow();

    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, VK_FORMAT_B8G8R8A8_SRGB);
    const VkExtent2D extent = chooseSwapExtent(window.getFramebufferSize(), swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface.getVkSurface(),
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    const QueueFamilyIndices indices = logicalDevice.getPhysicalDevice().getQueueFamilyIndices();
    const uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(std::size(queueFamilyIndices));
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain;
    if (VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain); result != VK_SUCCESS) {
        return Error(result);
    }

    vkGetSwapchainImagesKHR(logicalDevice.getVkDevice(), swapchain, &imageCount, nullptr);
    return std::unique_ptr<Swapchain>(new Swapchain(swapchain, logicalDevice, surfaceFormat, extent, imageCount));
}

VkResult Swapchain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const {
    return vkAcquireNextImageKHR(_logicalDevice.getVkDevice(), _swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
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

    return vkQueuePresentKHR(_logicalDevice.getPresentVkQueue(), &presentInfo);
}

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::span<const VkSurfaceFormatKHR> availableFormats, VkFormat preferredFormat) {
    auto availableFormat = std::find_if(std::cbegin(availableFormats), std::cend(availableFormats), [=](const auto& format) {
        return format.format == preferredFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
    return (availableFormat != std::cend(availableFormats)) ? *availableFormat : availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(std::span<const VkPresentModeKHR> availablePresentModes, VkPresentModeKHR preferredMode) {
    auto availablePresentMode = std::find_if(std::cbegin(availablePresentModes), std::cend(availablePresentModes), [=](const auto& mode) {
        return mode == preferredMode;
        });

    return (availablePresentMode != std::cend(availablePresentModes)) ? *availablePresentMode : VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    actualWindowExtent.width = std::clamp(actualWindowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualWindowExtent.height = std::clamp(actualWindowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualWindowExtent;
}

}   // namespace