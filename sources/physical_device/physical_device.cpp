#include "physical_device.h"

#include "lib/algorithm.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, std::unordered_set<std::string_view>&& availableRequestedExtensions, PhysicalDevicePropertyManager&& propertManager)
	: _device(physicalDevice), _surface(surface), _availableRequestedExtensions(std::move(availableRequestedExtensions)), _propertyManager(std::move(propertManager)) { }

ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::create(const Surface& surface) {
    const VkSurfaceKHR surf = surface.getVkSurface();

    ASSIGN_OR_RETURN(const lib::Buffer<VkPhysicalDevice> devices, surface.getInstance().getAvailablePhysicalDevices());
    for (const auto device : devices) {
        PhysicalDevicePropertyManager propertyManager(device, surf);
        const QueueFamilyIndices& indices = propertyManager.getQueueFamilyIndices();
        const SwapChainSupportDetails swapChainSupport = propertyManager.getSwapChainSupportDetails();

        bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        const bool discreteGPU = propertyManager.isDiscreteGPU();

        if (lib::cont_all_of(std::initializer_list{ indices.isComplete(), swapChainAdequate, static_cast<bool>(supportedFeatures.samplerAnisotropy), discreteGPU }, [](bool condition) { return condition; })) {
            return std::unique_ptr<PhysicalDevice>(new PhysicalDevice(device, surface, propertyManager.checkDeviceExtensionSupport(), std::move(propertyManager)));
        }
    }
    return Error(EngineError::NOT_FOUND);
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
