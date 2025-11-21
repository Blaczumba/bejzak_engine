#include "surface.h"

#include "vulkan/wrapper/util/check.h"

#if (defined(WIN32) || defined(__unix__)) && !defined(ANDROID)
  #include "common/window/window_glfw.h"
#endif

Surface::Surface(VkSurfaceKHR surface, const Instance& instance)
  : _surface(surface), _instance(&instance) {}

ErrorOr<Surface> Surface::create(const Instance& instance, const Window& window) {
#if (defined(WIN32) || defined(__unix__)) && !defined(ANDROID)
  if (dynamic_cast<const WindowGlfw*>(&window) != nullptr) {
    VkSurfaceKHR surface;
    CHECK_VKCMD(glfwCreateWindowSurface(
        instance.getVkInstance(), static_cast<GLFWwindow*>(window.getNativeHandler()), nullptr,
        &surface));
    return Surface(surface, instance);
  }
#endif
  return Error(EngineError::NOT_RECOGNIZED_TYPE);
}

Surface::Surface(Surface&& other) noexcept
  : _surface(std::exchange(other._surface, VK_NULL_HANDLE)),
    _instance(std::exchange(other._instance, nullptr)) {}

Surface& Surface::operator=(Surface&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  _surface = std::exchange(other._surface, VK_NULL_HANDLE);
  _instance = std::exchange(other._instance, nullptr);
  return *this;
}

Surface::~Surface() {
  if (_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(_instance->getVkInstance(), _surface, nullptr);
  }
}

VkSurfaceKHR Surface::getVkSurface() const {
  return _surface;
}
