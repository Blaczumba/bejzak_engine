#pragma once
#include "input.h"

#include "vulkan_lib/instance/instance.h"
#include "vulkan_lib/status/status.h"
#include "vulkan_lib/surface/surface.h"

#include "vulkan/vulkan.h"

#include <functional>
#include <memory>
#include <vector>

class Surface;

enum class WindowType : uint8_t {
	Automatic,
	Glfw
};

class Window {
public:
	static ErrorOr<std::unique_ptr<Window>> createWindow(std::string_view windowName, uint32_t width, uint32_t height, WindowType windowType = WindowType::Automatic);

	virtual ~Window() = default;

	virtual bool open() const = 0;
	virtual void close() const = 0;

	virtual void pollEvents() = 0;

	virtual void setWindowSize(int width, int height) = 0;
	virtual VkExtent2D getFramebufferSize() const = 0;

	virtual std::vector<const char*> getExtensions() const = 0;
	virtual ErrorOr<std::unique_ptr<Surface>> createSurface(const Instance& instance) const = 0;
	virtual MouseKeyboardManager* getMouseKeyboardManager() = 0;
};
