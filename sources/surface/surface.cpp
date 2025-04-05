#include "surface.h"

Surface::Surface(const VkSurfaceKHR surface, const Instance& instance, const Window& window) : _surface(surface), _instance(instance), _window(window) {}

Surface::~Surface() {
	vkDestroySurfaceKHR(_instance.getVkInstance(), _surface, nullptr);
}

const VkSurfaceKHR Surface::getVkSurface() const {
	return _surface;
}

const Instance& Surface::getInstance() const {
	return _instance;
}

const Window& Surface::getWindow() const {
	return _window;
}
