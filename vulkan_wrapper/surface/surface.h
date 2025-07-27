#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/instance/instance.h"
#include "common/window/window.h"

#include "vulkan/vulkan.h"

#include <memory>

class Surface {
	Surface(VkSurfaceKHR surface, const Instance& instance, const Window& window);

public:
	static ErrorOr<std::unique_ptr<Surface>> create(const Instance& instance, Window* window);

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