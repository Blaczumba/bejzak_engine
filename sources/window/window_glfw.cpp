#include "window_glfw.h"

#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>

namespace Keyboard {
namespace {

constexpr std::array<int16_t, 121> fromKeyToGlfw = {
    GLFW_KEY_UNKNOWN,

    GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O, GLFW_KEY_P,
    GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L,
    GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_B, GLFW_KEY_N, GLFW_KEY_M,

    GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,

    GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8,
    GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12, GLFW_KEY_F13, GLFW_KEY_F14, GLFW_KEY_F15, GLFW_KEY_F16,
    GLFW_KEY_F17, GLFW_KEY_F18, GLFW_KEY_F19, GLFW_KEY_F20, GLFW_KEY_F21, GLFW_KEY_F22, GLFW_KEY_F23, GLFW_KEY_F24, GLFW_KEY_F25,

    GLFW_KEY_SPACE, GLFW_KEY_APOSTROPHE, GLFW_KEY_COMMA, GLFW_KEY_MINUS, GLFW_KEY_PERIOD, GLFW_KEY_SLASH,
    GLFW_KEY_SEMICOLON, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_GRAVE_ACCENT,

    GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL,
    GLFW_KEY_LEFT_ALT, GLFW_KEY_RIGHT_ALT, GLFW_KEY_LEFT_SUPER, GLFW_KEY_RIGHT_SUPER, GLFW_KEY_CAPS_LOCK,

    GLFW_KEY_ESCAPE, GLFW_KEY_ENTER, GLFW_KEY_TAB, GLFW_KEY_BACKSPACE, GLFW_KEY_INSERT, GLFW_KEY_DELETE,
    GLFW_KEY_RIGHT, GLFW_KEY_LEFT, GLFW_KEY_DOWN, GLFW_KEY_UP,
    GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN, GLFW_KEY_HOME, GLFW_KEY_END,

    GLFW_KEY_SCROLL_LOCK, GLFW_KEY_NUM_LOCK, GLFW_KEY_PRINT_SCREEN, GLFW_KEY_PAUSE,

    GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4, GLFW_KEY_KP_5, GLFW_KEY_KP_6, GLFW_KEY_KP_7, GLFW_KEY_KP_8, GLFW_KEY_KP_9,
    GLFW_KEY_KP_DECIMAL, GLFW_KEY_KP_DIVIDE, GLFW_KEY_KP_MULTIPLY, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_ADD, GLFW_KEY_KP_ENTER, GLFW_KEY_KP_EQUAL,

    GLFW_KEY_WORLD_1, GLFW_KEY_WORLD_2,

    GLFW_KEY_MENU
};

constexpr std::array<Key, 349> fromGlfwToKey = [] {
    std::array<Key, GLFW_KEY_MENU + 1> mapping{};
    for (uint16_t i = 1; i < fromKeyToGlfw.size(); ++i) {
        mapping[fromKeyToGlfw[i]] = static_cast<Key>(i);
    }
    return mapping;
}();

} // namespace
} // namespace Keyboard

MouseKeyboardGlfw::MouseKeyboardGlfw(GLFWwindow* window) : _window(window) { }

bool MouseKeyboardGlfw::isPressed(Keyboard::Key key) const {
    return glfwGetKey(_window, Keyboard::fromKeyToGlfw[static_cast<uint16_t>(key)]) == GLFW_PRESS;
}

void MouseKeyboardGlfw::setKeyboardCallback(Keyboard::Callback callback) const {
    static Keyboard::Callback keyCallback = callback;
    glfwSetKeyCallback(_window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        keyCallback(Keyboard::fromGlfwToKey[key], action);
    });
}

void MouseKeyboardGlfw::setMouseMoveCallback(Mouse::MoveCallback callback) const {
    static Mouse::MoveCallback mouseCallback = callback;
    glfwSetCursorPosCallback(_window, [](GLFWwindow* win, double xPos, double yPos) {
        mouseCallback(xPos, yPos);
    });
}

WindowGlfw::WindowGlfw(std::string_view windowName, uint32_t width, uint32_t height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(width, height, windowName.data(), nullptr, nullptr);
    _mouseKeyboard = std::make_unique<MouseKeyboardGlfw>(_window);

    glfwSetWindowUserPointer(_window, this);
}

WindowGlfw::~WindowGlfw() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

GLFWwindow* WindowGlfw::getGlfwWindow() {
    return _window;
}

bool WindowGlfw::open() const {
    return !glfwWindowShouldClose(_window);
}

void WindowGlfw::close() const {
    glfwSetWindowShouldClose(_window, true);
}

void WindowGlfw::absorbCursor() const {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowGlfw::freeCursor() const {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void WindowGlfw::pollEvents() {
    glfwPollEvents();
}

void WindowGlfw::setWindowSize(int width, int height) {
    glfwSetWindowSize(_window, width, height);
}

VkExtent2D WindowGlfw::getFramebufferSize() const {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

std::vector<const char*> WindowGlfw::getExtensions() const {
    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

lib::ErrorOr<std::unique_ptr<Surface>> WindowGlfw::createSurface(const Instance& instance) const {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance.getVkInstance(), _window, nullptr, &surface) != VK_SUCCESS) {
        return lib::Error("Failed to create window surface.");
    }
    return std::unique_ptr<Surface>(new Surface(surface, instance, *this));
}

MouseKeyboard* WindowGlfw::getMouseKeyboard() {
    return _mouseKeyboard.get();
}
