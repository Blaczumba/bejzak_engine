#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/instance/instance.h"
#include "common/window/window.h"

#include "vulkan/vulkan.h"

#include <memory>

class Surface {
	Surface(VkSurfaceKHR surface, const Instance& instance, const Window& window);

public:
	Surface() = default;

	static ErrorOr<Surface> create(const Instance& instance, const Window& window);

	Surface(Surface&& other) noexcept;

	Surface& operator=(Surface&& other) noexcept;

	~Surface();

	VkSurfaceKHR getVkSurface() const;

	const Instance& getInstance() const;

	const Window& getWindow() const;

private:
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	const Instance* _instance = nullptr;
	const Window* _window = nullptr;
};