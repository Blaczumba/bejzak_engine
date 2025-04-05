#pragma once
#include "input.h"

#include "instance/instance.h"
#include "lib/status/status.h"
#include "surface/surface.h"

#include "vulkan/vulkan.h"

#include <functional>
#include <vector>

class Surface;

class Window {
public:
	virtual ~Window() = default;

	virtual bool open() const = 0;
	virtual void pollEvents() = 0;
	virtual void setWindowSize(int width, int height) = 0;
	virtual VkExtent2D getFramebufferSize() const = 0;
	virtual std::vector<const char*> getExtensions() const = 0;
	virtual lib::ErrorOr<std::unique_ptr<Surface>> createSurface(const Instance& instance) const = 0;
	virtual MouseKeyboard* getMouseKeyboard() = 0;
};
