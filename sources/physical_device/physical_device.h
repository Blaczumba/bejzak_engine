#pragma once

#include "lib/status/status.h"
#include "property_manager.h"
#include "surface/surface.h"

#include <memory>
#include <optional>
#include <vector>

class LogicalDevice;

class PhysicalDevice {
    const VkPhysicalDevice _device;
    const Surface& _surface;

    const PhysicalDevicePropertyManager _propertyManager;

	PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, PhysicalDevicePropertyManager&& propertManager);

public:
    ~PhysicalDevice() = default;

    static lib::ErrorOr<std::unique_ptr<PhysicalDevice>> create(const Surface& surface);

    const VkPhysicalDevice getVkPhysicalDevice() const;
    const Surface& getSurface() const;
    const PhysicalDevicePropertyManager& getPropertyManager() const;
};
