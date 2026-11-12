#include "engine/input_manager.h"

namespace engine {

InputManager::InputManager(common::Presentation& presentation) noexcept
  : _presentation(presentation) {}

void InputManager::setMouseKeyboardManager(
    std::unique_ptr<const MouseKeyboardManager> mouseKeyboardManager) noexcept {
  _mouseKeyboardManager = std::move(mouseKeyboardManager);
}

const MouseKeyboardManager* InputManager::getMouseKeyboardManager() const noexcept {
  return _mouseKeyboardManager.get();
}

}  // namespace engine
