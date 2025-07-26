#pragma once
#include "common/input_manager/mouse_keyboard_manager.h"

#include "common/util/types.h"

#include <functional>
#include <memory>
#include <vector>

class Window {
public:
	virtual ~Window() = default;

	virtual bool open() const = 0;
	virtual void close() const = 0;

	virtual void pollEvents() = 0;

	virtual void setWindowSize(int width, int height) = 0;
	virtual Extent2D getFramebufferSize() const = 0;

	virtual std::vector<const char*> getVulkanExtensions() const = 0;
	virtual void* getNativeHandler() const = 0;
	virtual MouseKeyboardManager* getMouseKeyboardManager() = 0;
};
