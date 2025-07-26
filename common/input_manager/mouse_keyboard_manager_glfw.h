#pragma once

#include "mouse_keyboard_manager.h"

#include "glfw/glfw3.h"

class MouseKeyboardManagerGlfw : public MouseKeyboardManager {
	GLFWwindow* _window;

public:
	MouseKeyboardManagerGlfw(GLFWwindow* window);

	bool isPressed(Keyboard::Key key) const override;
	void setKeyboardCallback(Keyboard::Callback callback) const override;
	void setMouseMoveCallback(Mouse::MoveCallback callback) const override;
	void absorbCursor() const override;
	void freeCursor() const override;
};
