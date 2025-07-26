#include "window_glfw.h"

#include "common/input_manager/mouse_keyboard_manager_glfw.h"

#include <GLFW/glfw3.h>

WindowGlfw::WindowGlfw(std::string_view windowName, uint32_t width, uint32_t height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(width, height, windowName.data(), /*glfwGetPrimaryMonitor()*/ nullptr, nullptr);
    _mouseKeyboard = std::make_unique<MouseKeyboardManagerGlfw>(_window);

    glfwSetWindowUserPointer(_window, this);
}

WindowGlfw::~WindowGlfw() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool WindowGlfw::open() const {
    return !glfwWindowShouldClose(_window);
}

void WindowGlfw::close() const {
    glfwSetWindowShouldClose(_window, true);
}

void WindowGlfw::pollEvents() {
    glfwPollEvents();
}

void WindowGlfw::setWindowSize(int width, int height) {
    glfwSetWindowSize(_window, width, height);
}

Extent2D WindowGlfw::getFramebufferSize() const {
    int width, height;
    glfwWaitEvents();
    glfwGetFramebufferSize(_window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

std::vector<const char*> WindowGlfw::getVulkanExtensions() const {
    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

void* WindowGlfw::getNativeHandler() const {
    return _window;
}

MouseKeyboardManager* WindowGlfw::getMouseKeyboardManager() {
    return _mouseKeyboard.get();
}
