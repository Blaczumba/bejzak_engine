#include "physical_device.h"

#include "logical_device/logical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physicalDevice, const Window& window, PhysicalDevicePropertyManager&& propertManager)
	: _device(physicalDevice), _window(window), _propertyManager(std::move(propertManager)) { }

lib::ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::create(const Window& window) {
    const VkSurfaceKHR surf = window.getVkSurfaceKHR();

    for (const auto device : window.getInstance().getAvailablePhysicalDevices()) {
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
            return std::unique_ptr<PhysicalDevice>(new PhysicalDevice(device, window, std::move(propertyManager)));
        }
    }
    return lib::Error("failed to find a suitable GPU!");
}

const VkPhysicalDevice PhysicalDevice::getVkPhysicalDevice() const {
    return _device;
}

const Window& PhysicalDevice::getWindow() const {
    return _window;
}

const PhysicalDevicePropertyManager& PhysicalDevice::getPropertyManager() const {
    return _propertyManager;
}

std::unique_ptr<LogicalDevice> PhysicalDevice::createLogicalDevice() {
    return std::make_unique<LogicalDevice>(*this);
}