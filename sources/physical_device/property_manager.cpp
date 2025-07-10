#include "property_manager.h"

#include "lib/buffer/buffer.h"
#include "instance/extensions.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <unordered_set>
#include <iostream>

PhysicalDevicePropertyManager::PhysicalDevicePropertyManager(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface) {
    _physicalDevice = physicalDevice;
    _surface = surface;

    vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);

    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memoryProperties);

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

    _queueFamilies = lib::Buffer<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, _queueFamilies.data());

    _queueFamilyIndices = findQueueFamilyIncides();
}

QueueFamilyIndices PhysicalDevicePropertyManager::findQueueFamilyIncides() const {
    QueueFamilyIndices indices;

    const lib::Buffer<VkQueueFamilyProperties>& queueFamilies = getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size() && !indices.isComplete(); i++) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface, &presentSupport);

        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }

        if (presentSupport) {
            indices.presentFamily = i;
        }
    }

    return indices;
}

VkSampleCountFlagBits PhysicalDevicePropertyManager::getMaxUsableSampleCount() const {
    const VkSampleCountFlags counts = _properties.limits.framebufferColorSampleCounts & _properties.limits.framebufferDepthSampleCounts;

    // Sorted msaa samples.
    static constexpr std::array samples = {
        VK_SAMPLE_COUNT_64_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_2_BIT,
        VK_SAMPLE_COUNT_1_BIT
    };

    return *std::find_if(samples.cbegin(), samples.cend(), [&](VkSampleCountFlagBits sample) { return sample & counts; });
}

float PhysicalDevicePropertyManager::getMaxSamplerAnisotropy() const {
    return _properties.limits.maxSamplerAnisotropy;
}

const VkPhysicalDeviceLimits& PhysicalDevicePropertyManager::getPhysicalDeviceLimits() const {
    return _properties.limits;
}

SwapChainSupportDetails PhysicalDevicePropertyManager::querySwapchainSupportDetails() const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats = lib::Buffer<VkSurfaceFormatKHR>(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes = lib::Buffer<VkPresentModeKHR>(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}


const QueueFamilyIndices& PhysicalDevicePropertyManager::getQueueFamilyIndices() const {
    return _queueFamilyIndices;
}

SwapChainSupportDetails PhysicalDevicePropertyManager::getSwapChainSupportDetails() const {
    return querySwapchainSupportDetails();
}

const lib::Buffer<VkQueueFamilyProperties>& PhysicalDevicePropertyManager::getQueueFamilyProperties() const {
    return _queueFamilies;
}

bool PhysicalDevicePropertyManager::isDiscreteGPU() const {
    return _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

bool QueueFamilyIndices::isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
}
