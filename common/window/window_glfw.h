#pragma once

#include "window.h"

#include "common/input_manager/mouse_keyboard_manager_glfw.h"

#include <memory>
#include <string_view>

class WindowGlfw : public Window {
	GLFWwindow* _window;

	std::unique_ptr<MouseKeyboardManagerGlfw> _mouseKeyboard;

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
	MouseKeyboardManager* getMouseKeyboardManager() override;
};
