#pragma once

#include "window.h"

#include <GLFW/glfw3.h>

#include <memory>
#include <string_view>

class MouseKeyboardManagerGlfw;

class WindowGlfw : public Window, public std::enable_shared_from_this<const WindowGlfw> {
	GLFWwindow* _window;

public:
	WindowGlfw(std::string_view windowName, uint32_t width, uint32_t height);

	~WindowGlfw() override;

	bool open() const override;

	void close() const override;

	void pollEvents() override;

	void setWindowSize(int width, int height) override;

	Extent2D getFramebufferSize() const override;

	std::vector<const char*> getVulkanExtensions() const override;

	void* getNativeHandler() const override;

	std::unique_ptr<const MouseKeyboardManager> createMouseKeyboardManager() const override;
};
