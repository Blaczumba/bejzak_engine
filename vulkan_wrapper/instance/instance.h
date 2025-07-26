#pragma once

#include "vulkan_wrapper/lib/buffer/buffer.h"
#include "vulkan_wrapper/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <span>
#include <string_view>

class Instance {
	VkInstance _instance;

	Instance(VkInstance instance);

public:
	~Instance();

	static ErrorOr<std::unique_ptr<Instance>> create(std::string_view engineName, std::span<const char* const> requiredExtensions);

	VkInstance getVkInstance() const;
	ErrorOr<lib::Buffer<VkPhysicalDevice>> getAvailablePhysicalDevices() const;
};