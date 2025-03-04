#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <string_view>

class Instance {
	const VkInstance _instance;

	static bool checkValidationLayerSupport();

	Instance(const VkInstance instance);

public:
	~Instance();

	static lib::ErrorOr<std::unique_ptr<Instance>> create(std::string_view engineName, const std::vector<const char*>& requiredExtensions);

	const VkInstance getVkInstance() const;
	lib::ErrorOr<lib::Buffer<VkPhysicalDevice>> getAvailablePhysicalDevices() const;
};