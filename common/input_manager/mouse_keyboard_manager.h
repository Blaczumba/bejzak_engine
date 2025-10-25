#pragma once

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace Keyboard {

enum class Key : uint16_t {
  None,

  // Alphanumeric Keys
  Q,
  W,
  E,
  R,
  T,
  Y,
  U,
  I,
  O,
  P,
  A,
  S,
  D,
  F,
  G,
  H,
  J,
  K,
  L,
  Z,
  X,
  C,
  V,
  B,
  N,
  M,

  // Number Keys (Top Row)
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,

  // Function Keys
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  F25,

  // Special Characters and Symbols
  Space,
  Apostrophe,
  Comma,
  Minus,
  Period,
  Slash,
  Semicolon,
  Equal,
  LeftBracket,
  Backslash,
  RightBracket,
  GraveAccent,

  // Modifier Keys
  LeftShift,
  RightShift,
  LeftControl,
  RightControl,
  LeftAlt,
  RightAlt,
  LeftSuper,
  RightSuper,
  CapsLock,

  // Navigation Keys
  Escape,
  Enter,
  Tab,
  Backspace,
  Insert,
  Delete,
  Right,
  Left,
  Down,
  Up,
  PageUp,
  PageDown,
  Home,
  End,

  // Lock Keys
  ScrollLock,
  NumLock,
  PrintScreen,
  Pause,

  // Numpad Keys
  Numpad0,
  Numpad1,
  Numpad2,
  Numpad3,
  Numpad4,
  Numpad5,
  Numpad6,
  Numpad7,
  Numpad8,
  Numpad9,
  NumpadDecimal,
  NumpadDivide,
  NumpadMultiply,
  NumpadSubtract,
  NumpadAdd,
  NumpadEnter,
  NumpadEqual,

  // Additional Symbols and Signs
  World1,
  World2,

  // Media Keys
  MediaPlayPause,
  MediaStop,
  MediaNextTrack,
  MediaPrevTrack,
  VolumeUp,
  VolumeDown,
  Mute,

  // Application Keys
  Menu
};

using Callback = std::function<void(Key key, int action)>;

}  // namespace Keyboard

namespace Mouse {

enum class Button : uint8_t {
  None,

  Button1,
  Button2,
  Button3,
  Button4,
  Button5,
  Button6,
  Button7
};

using MoveCallback = std::function<void(float xPos, float yPos)>;
using ButtonCallback = std::function<void(Button button, int action)>;
using ScrollCallback = std::function<void(Button button, int action)>;

}  // namespace Mouse

class MouseKeyboardManager {
public:
  virtual ~MouseKeyboardManager() = default;

  virtual bool isPressed(Keyboard::Key key) const = 0;

  virtual glm::vec2 getMousePosition() const = 0;

  virtual void setKeyboardCallback(Keyboard::Callback callback) const = 0;

  virtual void setMouseMoveCallback(Mouse::MoveCallback callback) const = 0;

  virtual void absorbCursor() const = 0;

  virtual void freeCursor() const = 0;
};
