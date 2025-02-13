#include "physical_device.h"

#include "logical_device/logical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const Window& window)
	: _window(window) {
    const std::vector<VkPhysicalDevice> devices = _window.getInstance().getAvailablePhysicalDevices();
    const VkSurfaceKHR surf = _window.getVkSurfaceKHR();

    for (const auto device : devices) {
        _propertyManager.initiate(device, surf);
        const QueueFamilyIndices& indices = _propertyManager.getQueueFamilyIndices();
        const bool extensionsSupported = _propertyManager.checkDeviceExtensionSupport();

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            const SwapChainSupportDetails swapChainSupport = _propertyManager.getSwapChainSupportDetails();
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        bool discreteGPU = _propertyManager.isDiscreteGPU();

        const std::array conditions = {
            indices.isComplete(),
            extensionsSupported,
            swapChainAdequate,
            static_cast<bool>(supportedFeatures.samplerAnisotropy),
            discreteGPU
        };

        bool suitable = std::all_of(conditions.cbegin(), conditions.cend(), [](bool condition) { return condition; });
        if (suitable) {
            _device = device;
            break;
        }
    }

    if (_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
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