#pragma once

#include "window.h"

#include "common/input_manager/mouse_keyboard_manager_glfw.h"
#include "vulkan_wrapper/status/status.h"

#include <memory>
#include <string_view>


class WindowGlfw : public Window {
	GLFWwindow* _window;

	std::unique_ptr<MouseKeyboardManagerGlfw> _mouseKeyboard;

public:
	WindowGlfw(std::string_view windowName, uint32_t width, uint32_t height);
	~WindowGlfw() override;

	GLFWwindow* getGlfwWindow();

	bool open() const override;
	void close() const override;

	void pollEvents() override;

	void setWindowSize(int width, int height) override;
	VkExtent2D getFramebufferSize() const override;

	std::vector<const char*> getExtensions() const override;
	ErrorOr<std::unique_ptr<Surface>> createSurface(const Instance& instance) const override;
	MouseKeyboardManager* getMouseKeyboardManager() override;
};
