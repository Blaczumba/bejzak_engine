#include "window.h"

#ifdef WIN32
#include "window_glfw.h"
#endif // WIN32

#ifdef UNIX
#include "window_glfw.h"
#endif // UNIX

#include <memory>

lib::ErrorOr<std::unique_ptr<Window>> Window::createWindow(std::string_view windowName, uint32_t width, uint32_t height, WindowType windowType) {
	switch (windowType)
	{
	case WindowType::Automatic:
#if defined(WIN32) || defined(UNIX)
	case WindowType::Glfw:
		return std::make_unique<WindowGlfw>(windowName, width, height);
#endif
	default:
		return lib::Error("Could not create window");
	}
}