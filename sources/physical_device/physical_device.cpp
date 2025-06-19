#include "physical_device.h"

#include "logical_device/logical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, std::unordered_set<std::string_view>&& availableRequestedExtensions, PhysicalDevicePropertyManager&& propertManager)
	: _device(physicalDevice), _surface(surface), _availableRequestedExtensions(std::move(availableRequestedExtensions)), _propertyManager(std::move(propertManager)) { }

lib::ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::create(const Surface& surface) {
    const VkSurfaceKHR surf = surface.getVkSurface();

    ASSIGN_OR_RETURN(const lib::Buffer<VkPhysicalDevice> devices, surface.getInstance().getAvailablePhysicalDevices());
    for (const auto device : devices) {
        PhysicalDevicePropertyManager propertyManager(device, surf);
        const QueueFamilyIndices& indices = propertyManager.getQueueFamilyIndices();

        bool swapChainAdequate = false;
        const SwapChainSupportDetails swapChainSupport = propertyManager.getSwapChainSupportDetails();
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        bool discreteGPU = propertyManager.isDiscreteGPU();

        const std::array conditions = {
            indices.isComplete(),
            swapChainAdequate,
            static_cast<bool>(supportedFeatures.samplerAnisotropy),
            // discreteGPU
        };

        if (std::all_of(conditions.cbegin(), conditions.cend(), [](bool condition) { return condition; })) {
            return std::unique_ptr<PhysicalDevice>(new PhysicalDevice(device, surface, propertyManager.checkDeviceExtensionSupport(), std::move(propertyManager)));
        }
    }
    return lib::Error("failed to find a suitable GPU!");
}

const VkPhysicalDevice PhysicalDevice::getVkPhysicalDevice() const {
    return _device;
}

const Surface& PhysicalDevice::getSurface() const {
    return _surface;
}

const PhysicalDevicePropertyManager& PhysicalDevice::getPropertyManager() const {
    return _propertyManager;
}

const std::unordered_set<std::string_view>& PhysicalDevice::getAvailableRequestedExtensions() const {
    return _availableRequestedExtensions;
}
