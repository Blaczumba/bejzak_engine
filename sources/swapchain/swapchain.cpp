#include "swapchain.h"

#include "logical_device/logical_device.h"
#include "memory_objects/texture/texture.h"
#include "physical_device/physical_device.h"
#include "window/window/window.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace {

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat preferredFormat);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredMode);
VkExtent2D chooseSwapExtent(VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities);

}   // namespace

Swapchain::Swapchain(const LogicalDevice& logicalDevice)
	: _logicalDevice(logicalDevice) {
    create();
}

Swapchain::~Swapchain() {
    cleanup();
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

const std::vector<VkImage>& Swapchain::getVkImages() const {
    return _images;
}

const std::vector<VkImageView>& Swapchain::getVkImageViews() const {
    return _views;
}

void Swapchain::cleanup() {
    const VkDevice device = _logicalDevice.getVkDevice();

    for (const VkImageView view : _views) {
        vkDestroyImageView(device, view, nullptr);
    }

    vkDestroySwapchainKHR(device, _swapchain, nullptr);
}

void Swapchain::create() {
    const auto& propertyManager = _logicalDevice.getPhysicalDevice().getPropertyManager();

    const SwapChainSupportDetails swapChainSupport = propertyManager.getSwapChainSupportDetails();
    const VkDevice device = _logicalDevice.getVkDevice();
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, VK_PRESENT_MODE_MAILBOX_KHR);
    const Window& window = _logicalDevice.getPhysicalDevice().getWindow();

    _surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, VK_FORMAT_B8G8R8A8_SRGB);
    _extent = chooseSwapExtent(window.getFramebufferSize(), swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = window.getVkSurfaceKHR(),

        .minImageCount = imageCount,
        .imageFormat = _surfaceFormat.format,
        .imageColorSpace = _surfaceFormat.colorSpace,
        .imageExtent = _extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    const QueueFamilyIndices& indices = propertyManager.getQueueFamilyIndices();
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr);
    _images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, _images.data());
    _views.reserve(imageCount);
    std::transform(_images.cbegin(), _images.cend(), std::back_inserter(_views),
        [this](const VkImage image) {
            return _logicalDevice.createImageView(image, ImageParameters{
                    .format = _surfaceFormat.format,
                    .width = _extent.width,
                    .height = _extent.height,
                    .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                }
            );
        }
    );
}

std::unique_ptr<Swapchain> Swapchain::create(const LogicalDevice& logicalDevice) {
    return std::unique_ptr<Swapchain>(new Swapchain(logicalDevice));
}

void Swapchain::recrete() {
    cleanup();
    create();
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

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat preferredFormat) {
    auto availableFormat = std::find_if(availableFormats.cbegin(), availableFormats.cend(), [=](const auto& format) {
        return format.format == preferredFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
    return (availableFormat != availableFormats.cend()) ? *availableFormat : availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredMode) {
    auto availablePresentMode = std::find_if(availablePresentModes.cbegin(), availablePresentModes.cend(), [=](const auto& mode) {
        return mode == preferredMode;
        });

    return (availablePresentMode != availablePresentModes.cend()) ? *availablePresentMode : VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(VkExtent2D actualWindowExtent, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        actualWindowExtent.width = std::clamp(actualWindowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualWindowExtent.height = std::clamp(actualWindowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualWindowExtent;
    }
}

}   // namespace