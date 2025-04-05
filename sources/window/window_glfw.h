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
	void setKeyboardCallback(Keyboard::Callback callback) const override;
	void setMouseMoveCallback(Mouse::MoveCallback callback) const override;
};

class WindowGlfw : public Window {
	GLFWwindow* _window;

	std::unique_ptr<MouseKeyboardGlfw> _mouseKeyboard;

public:
	WindowGlfw(std::string_view windowName, uint32_t width, uint32_t height);
	~WindowGlfw() override;

	GLFWwindow* getGlfwWindow();

	bool open() const override;
	void close() const override;
	void absorbCursor() const override;
	void freeCursor() const override;

	void pollEvents() override;

	void setWindowSize(int width, int height) override;
	VkExtent2D getFramebufferSize() const override;

	std::vector<const char*> getExtensions() const override;
	lib::ErrorOr<std::unique_ptr<Surface>> createSurface(const Instance& instance) const override;
	MouseKeyboard* getMouseKeyboard() override;
};
