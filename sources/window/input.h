#pragma once

#include <functional>

namespace Keyboard {

enum class Key : uint16_t {
    None,

    // Alphanumeric Keys
    Q, W, E, R, T, Y, U, I, O, P,
    A, S, D, F, G, H, J, K, L,
    Z, X, C, V, B, N, M,

    // Number Keys (Top Row)
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function Keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,

    // Special Characters and Symbols
    Space, Apostrophe, Comma, Minus, Period, Slash,
    Semicolon, Equal, LeftBracket, Backslash, RightBracket, GraveAccent,

    // Modifier Keys
    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper, CapsLock,

    // Navigation Keys
    Escape, Enter, Tab, Backspace, Insert, Delete, Right, Left, Down, Up,
    PageUp, PageDown, Home, End,

    // Lock Keys
    ScrollLock, NumLock, PrintScreen, Pause,

    // Numpad Keys
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter, NumpadEqual,

    // Additional Symbols and Signs
    World1, World2,

    // Media Keys
    MediaPlayPause, MediaStop, MediaNextTrack, MediaPrevTrack,
    VolumeUp, VolumeDown, Mute,

    // Application Keys
    Menu
};

}

class InputDevice {

};

using InputCallback = std::function<void(Keyboard::Key key, int action)>;

class MouseKeyboard : public InputDevice {
public:
	virtual bool isPressed(Keyboard::Key key) const = 0;
	virtual void setInputCallback(InputCallback callback) const = 0;
    // virtual void setMouseCallback(MouseCallback callback)
    virtual std::pair<float, float> getMouseOffsets() const = 0;
};
