#include "physical_device.h"

#include "logical_device/logical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, PhysicalDevicePropertyManager&& propertManager)
	: _device(physicalDevice), _surface(surface), _propertyManager(std::move(propertManager)) { }

lib::ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::create(const Surface& surface) {
    const VkSurfaceKHR surf = surface.getVkSurface();

    ASSIGN_OR_RETURN(const lib::Buffer<VkPhysicalDevice> devices, surface.getInstance().getAvailablePhysicalDevices());
    for (const auto device : devices) {
        PhysicalDevicePropertyManager propertyManager(device, surf);
        const QueueFamilyIndices& indices = propertyManager.getQueueFamilyIndices();
        const bool extensionsSupported = propertyManager.checkDeviceExtensionSupport();

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            const SwapChainSupportDetails swapChainSupport = propertyManager.getSwapChainSupportDetails();
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        bool discreteGPU = propertyManager.isDiscreteGPU();

        const std::array conditions = {
            indices.isComplete(),
            extensionsSupported,
            swapChainAdequate,
            static_cast<bool>(supportedFeatures.samplerAnisotropy),
            discreteGPU
        };

        if (std::all_of(conditions.cbegin(), conditions.cend(), [](bool condition) { return condition; })) {
            return std::unique_ptr<PhysicalDevice>(new PhysicalDevice(device, surface, std::move(propertyManager)));
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
