#pragma once

#include "lib/status/status.h"
#include "property_manager.h"
#include "window/window/window.h"

#include <memory>
#include <optional>
#include <vector>

class LogicalDevice;

class PhysicalDevice {
    const VkPhysicalDevice _device;
    const Window& _window;

    const PhysicalDevicePropertyManager _propertyManager;

	PhysicalDevice(const VkPhysicalDevice physicalDevice, const Window& window, PhysicalDevicePropertyManager&& propertManager);

public:
    ~PhysicalDevice() = default;

    static lib::ErrorOr<std::unique_ptr<PhysicalDevice>> create(const Window& window);

    const VkPhysicalDevice getVkPhysicalDevice() const;
    const Window& getWindow() const;
    const PhysicalDevicePropertyManager& getPropertyManager() const;

    std::unique_ptr<LogicalDevice> createLogicalDevice();
};
