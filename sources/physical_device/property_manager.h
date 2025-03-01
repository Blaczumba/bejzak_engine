#pragma once

#include <vulkan/vulkan.h>

#include <optional>
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
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class PhysicalDevicePropertyManager {
    VkPhysicalDevice _physicalDevice    = VK_NULL_HANDLE;
    VkSurfaceKHR _surface               = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties _properties              = {};
    VkPhysicalDeviceMemoryProperties _memoryProperties  = {};
    std::vector<VkExtensionProperties> _availableExtensions;
    std::vector<VkQueueFamilyProperties> _queueFamilies;

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

    const std::vector<VkQueueFamilyProperties>& getQueueFamilyProperties() const;
    const std::vector<VkExtensionProperties>& getAvailableExtensionProperties() const;

    bool isDiscreteGPU() const;
    bool checkDeviceExtensionSupport() const;

    bool checkBlittingSupport(VkFormat format) const;
    bool checkTextureFormatSupport(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    friend class PhysicalDevice;
};

