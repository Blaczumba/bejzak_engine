#pragma once

#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>

class Instance;

class DebugMessenger {
	VkDebugUtilsMessengerEXT _debugUtilsMessenger;

	const Instance& _instance;

	DebugMessenger(const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger);
public:
	static ErrorOr<std::unique_ptr<DebugMessenger>> create(const Instance& instance);

	~DebugMessenger();
};