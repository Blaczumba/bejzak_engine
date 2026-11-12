#pragma once

#include <bitset>

#include "common/abstractions/presentation.h"
#include "common/input_manager/mouse_keyboard_manager.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace engine {

class InputManager {
public:
  InputManager(common::Presentation& presentation) noexcept;

  void setMouseKeyboardManager(
      std::unique_ptr<const MouseKeyboardManager> mouseKeyboardManager) noexcept;

  const MouseKeyboardManager* getMouseKeyboardManager() const noexcept;

  // TODO: Set and get other managers like gamepad, joystick etc.

  // Gets or registers new action of type T from
  // "/presentation_graphics_communication/input_manager.h".
  template <typename T>
  std::optional<T>* registerListeningFor();

private:
  common::Presentation& _presentation;

  std::unique_ptr<const MouseKeyboardManager> _mouseKeyboardManager;
};

template <typename T>
std::optional<T>* InputManager::registerListeningFor() {
  return _presentation.registerAction<T>();
}

}  // namespace engine
