#pragma once

#include "common/input_manager/mouse_keyboard_manager.h"

class MouseKeyboardCameraController {
public:
    virtual void updateFromKeyboard(const MouseKeyboardManager& mouseKeyboardManager, float xOffset, float yOffset, float deltaTime) = 0;
};