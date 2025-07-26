#pragma once

#include "mouse_keyboard_manager.h"
#include "common/window/window_glfw.h"

#include "glfw/glfw3.h"

#include <memory>

class WindowGlfw;

class MouseKeyboardManagerGlfw : public MouseKeyboardManager {
	std::shared_ptr<const Window> _window;

public:
	MouseKeyboardManagerGlfw(const std::shared_ptr<const WindowGlfw>& window);

	bool isPressed(Keyboard::Key key) const override;

	void setKeyboardCallback(Keyboard::Callback callback) const override;

	void setMouseMoveCallback(Mouse::MoveCallback callback) const override;

	void absorbCursor() const override;

	void freeCursor() const override;
};
