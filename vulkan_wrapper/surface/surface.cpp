#include "surface.h"

#if (defined(WIN32) || defined(__unix__)) && !defined(ANDROID)
#include "common/window/window_glfw.h"
#endif

Surface::Surface(VkSurfaceKHR surface, const Instance& instance, const Window& window) : _surface(surface), _instance(instance), _window(window) {}

ErrorOr<std::unique_ptr<Surface>> Surface::create(const Instance& instance, Window* window) {
#if (defined(WIN32) || defined(__unix__)) && !defined(ANDROID)
	if (dynamic_cast<WindowGlfw*>(window) != nullptr) {
		VkSurfaceKHR surface;
		if (VkResult result = glfwCreateWindowSurface(instance.getVkInstance(), static_cast<GLFWwindow*>(window->getNativeHandler()), nullptr, &surface); result != VK_SUCCESS) {
			return Error(result);
		}
		return std::unique_ptr<Surface>(new Surface(surface, instance, *window));
	}
#endif
	return Error(EngineError::NOT_RECOGNIZED_TYPE);
}

Surface::~Surface() {
	vkDestroySurfaceKHR(_instance.getVkInstance(), _surface, nullptr);
}

VkSurfaceKHR Surface::getVkSurface() const {
	return _surface;
}

const Instance& Surface::getInstance() const {
	return _instance;
}

const Window& Surface::getWindow() const {
	return _window;
}
