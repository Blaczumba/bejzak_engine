#pragma once

#include "vulkan_lib/instance/instance.h"
#include "vulkan_lib/window/window.h"

#include "vulkan/vulkan.h"

class Window;

class Surface {
	Surface(VkSurfaceKHR surface, const Instance& instance, const Window& window);

public:
	~Surface();

	VkSurfaceKHR getVkSurface() const;
	const Instance& getInstance() const;
	const Window& getWindow() const;

private:
	VkSurfaceKHR _surface;
	const Instance& _instance;
	const Window& _window;

	// TODO remove
	friend class WindowGlfw;
};