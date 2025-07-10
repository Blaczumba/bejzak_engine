#include "swapchain.h"

#include "logical_device/logical_device.h"
#include "memory_objects/texture.h"
#include "physical_device/physical_device.h"
#include "window/window.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const lib::Buffer<VkSurfaceFormatKHR>& availableFormats, VkFormat preferredFormat);
VkPresentModeKHR chooseSwapPresentMode(const lib::Buffer<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredMode);
VkExtent2D chooseSwapExtent(VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities);

}   // namespace

Swapchain::Swapchain(const VkSwapchainKHR swapchain, const LogicalDevice& logicalDevice, VkSurfaceFormatKHR surfaceFormat, VkExtent2D extent, lib::Buffer<VkImage>&& images, lib::Buffer<VkImageView>&& views)
	: _swapchain(swapchain), _logicalDevice(logicalDevice), _surfaceFormat(surfaceFormat), _extent(extent), _images(std::move(images)), _views(std::move(views)) { }

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
    return _views[index];
}

ErrorOr<std::unique_ptr<Swapchain>> Swapchain::create(const LogicalDevice& logicalDevice, const VkSwapchainKHR oldSwapchain) {
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
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
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

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    lib::Buffer<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    lib::Buffer<VkImageView> views(imageCount);
    std::transform(images.cbegin(), images.cend(), views.begin(),
        [&](const VkImage image) {
            return logicalDevice.createImageView(image, ImageParameters{
                    .format = surfaceFormat.format,
                    .width = extent.width,
                    .height = extent.height,
                    .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                }
            );
        }
    );
    return std::unique_ptr<Swapchain>(new Swapchain(swapchain, logicalDevice, surfaceFormat, extent, std::move(images), std::move(views)));
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

    return vkQueuePresentKHR(_logicalDevice.getPresentQueue(), &presentInfo);
}

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const lib::Buffer<VkSurfaceFormatKHR>& availableFormats, VkFormat preferredFormat) {
    auto availableFormat = std::find_if(availableFormats.cbegin(), availableFormats.cend(), [=](const auto& format) {
        return format.format == preferredFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
    return (availableFormat != availableFormats.cend()) ? *availableFormat : availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const lib::Buffer<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredMode) {
    auto availablePresentMode = std::find_if(availablePresentModes.cbegin(), availablePresentModes.cend(), [=](const auto& mode) {
        return mode == preferredMode;
        });

    return (availablePresentMode != availablePresentModes.cend()) ? *availablePresentMode : VK_PRESENT_MODE_FIFO_KHR;
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