#pragma once

#include <vulkan/vulkan.h>

#include <memory>

class Instance;

class DebugMessenger {
	VkDebugUtilsMessengerEXT _debugMessenger;
	const Instance& _instance;

	VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);

public:
	DebugMessenger(const Instance& instance);
	~DebugMessenger();
};