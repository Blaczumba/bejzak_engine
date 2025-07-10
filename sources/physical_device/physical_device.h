#pragma once

#include "property_manager.h"
#include "status/status.h"
#include "surface/surface.h"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

class LogicalDevice;

class PhysicalDevice {
    const VkPhysicalDevice _device;
    const Surface& _surface;

    const PhysicalDevicePropertyManager _propertyManager;
    const std::unordered_set<std::string_view> _availableRequestedExtensions; // TODO: change to flat hash set

	PhysicalDevice(const VkPhysicalDevice physicalDevice, const Surface& surface, std::unordered_set<std::string_view>&& availableRequestedExtensions, PhysicalDevicePropertyManager&& propertManager);

public:
    ~PhysicalDevice() = default;

    static ErrorOr<std::unique_ptr<PhysicalDevice>> create(const Surface& surface);

    const VkPhysicalDevice getVkPhysicalDevice() const;
    const Surface& getSurface() const;
    const PhysicalDevicePropertyManager& getPropertyManager() const;
    bool hasAvailableExtension(std::string_view extension) const;
    lib::Buffer<const char*> getAvailableExtensions() const;
};
