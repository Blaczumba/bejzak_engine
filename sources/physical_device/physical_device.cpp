#include "physical_device.h"

#include "lib/algorithm.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "instance/extensions.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <vulkan/vulkan.hpp>

PhysicalDevice::PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, std::unordered_set<std::string_view>&& availableRequestedExtensions, PhysicalDevicePropertyManager&& propertManager)
	: _device(physicalDevice), _surface(surface), _availableRequestedExtensions(std::move(availableRequestedExtensions)), _propertyManager(std::move(propertManager)) { }

namespace {

std::unordered_set<std::string_view> checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    lib::Buffer<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::unordered_set<std::string_view> availableExtensionNames;
    availableExtensionNames.reserve(extensionCount);
    for (const VkExtensionProperties& ext : availableExtensions) {
        availableExtensionNames.emplace(ext.extensionName);
    }

    std::unordered_set<std::string_view> supportedRequestedExtensions;
    for (const char* requested : requestedDeviceExtensions) {
        if (availableExtensionNames.contains(requested)) {
            supportedRequestedExtensions.emplace(requested);
        }
    }

    return supportedRequestedExtensions;
}

} // namespace

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
            return std::unique_ptr<PhysicalDevice>(new PhysicalDevice(device, surface, checkDeviceExtensionSupport(device), std::move(propertyManager)));
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

bool PhysicalDevice::hasAvailableExtension(std::string_view extension) const {
    return _availableRequestedExtensions.contains(extension);
}

lib::Buffer<const char*> PhysicalDevice::getAvailableExtensions() const {
    lib::Buffer<const char*> extensions(_availableRequestedExtensions.size());
    std::transform(_availableRequestedExtensions.cbegin(), _availableRequestedExtensions.cend(),
        extensions.begin(),[](std::string_view extension) { return extension.data(); });
    return extensions;
}
