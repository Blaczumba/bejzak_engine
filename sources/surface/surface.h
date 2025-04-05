#pragma once

#include "instance/instance.h"
#include "window/window.h"

#include "vulkan/vulkan.h"

class Window;

class Surface {
	Surface(const VkSurfaceKHR surface, const Instance& instance, const Window& window);

public:
	~Surface();

	const VkSurfaceKHR getVkSurface() const;
	const Instance& getInstance() const;
	const Window& getWindow() const;

private:
	VkSurfaceKHR _surface;
	const Instance& _instance;
	const Window& _window;

	// TODO remove
	friend class WindowGLFW;
};