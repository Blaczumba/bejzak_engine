#pragma once

#include "window.h"
#include "input.h"

#include <memory>
#include <string_view>

class GLFWwindow;

class MouseKeyboardGlfw : public MouseKeyboard {
	GLFWwindow* _window;

public:
	MouseKeyboardGlfw(GLFWwindow* window);

	bool isPressed(Keyboard::Key key) const override;
	void setInputCallback(InputCallback callback) const override;
	std::pair<float, float> getMouseOffsets() const override;
};

class WindowGLFW : public Window {
	GLFWwindow* _window;

	std::unique_ptr<MouseKeyboardGlfw> _mouseKeyboard;

public:
	WindowGLFW(std::string_view windowName, uint32_t width, uint32_t height);
	~WindowGLFW() override;

	GLFWwindow* getGlfwWindow();

	bool open() const override;
	void pollEvents() override;
	void setWindowSize(int width, int height) override;
	VkExtent2D getFramebufferSize() const override;
	std::vector<const char*> getExtensions() const override;
	lib::ErrorOr<std::unique_ptr<Surface>> createSurface(const Instance& instance) const override;
	MouseKeyboard* getMouseKeyboard() override;
};
