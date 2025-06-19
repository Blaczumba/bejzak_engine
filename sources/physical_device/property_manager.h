#pragma once

#include "lib/buffer/buffer.h"

#include <vulkan/vulkan.h>

#include <optional>
#include <string_view>
#include <unordered_set>
#include <vector>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() const;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    lib::Buffer<VkSurfaceFormatKHR> formats;
    lib::Buffer<VkPresentModeKHR> presentModes;
};

class PhysicalDevicePropertyManager {
    VkPhysicalDevice _physicalDevice    = VK_NULL_HANDLE;
    VkSurfaceKHR _surface               = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties _properties              = {};
    VkPhysicalDeviceMemoryProperties _memoryProperties  = {};
    lib::Buffer<VkExtensionProperties> _availableExtensions;
    lib::Buffer<VkQueueFamilyProperties> _queueFamilies;

    QueueFamilyIndices _queueFamilyIndices;

    QueueFamilyIndices findQueueFamilyIncides() const;
    SwapChainSupportDetails querySwapchainSupportDetails() const;

public:
    PhysicalDevicePropertyManager(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface);

    VkSampleCountFlagBits getMaxUsableSampleCount() const;
    float getMaxSamplerAnisotropy() const;

    const VkPhysicalDeviceLimits& getPhysicalDeviceLimits() const;

    const QueueFamilyIndices& getQueueFamilyIndices() const;
    SwapChainSupportDetails getSwapChainSupportDetails() const;

    const lib::Buffer<VkQueueFamilyProperties>& getQueueFamilyProperties() const;
    const lib::Buffer<VkExtensionProperties>& getAvailableExtensionProperties() const;

    bool isDiscreteGPU() const;
    std::unordered_set<std::string_view> checkDeviceExtensionSupport() const;

    bool checkBlittingSupport(VkFormat format) const;
    bool checkTextureFormatSupport(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    friend class PhysicalDevice;
};

