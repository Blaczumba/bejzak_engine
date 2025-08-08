#pragma once

#include <memory>

#include "common/status/status.h"
#include "common/window/window.h"
#include "vulkan/vulkan.h"
#include "vulkan_wrapper/instance/instance.h"

class Surface {
  Surface(VkSurfaceKHR surface, const Instance& instance);

public:
  Surface() = default;

  static ErrorOr<Surface> create(const Instance& instance, const Window& window);

  Surface(Surface&& other) noexcept;

  Surface& operator=(Surface&& other) noexcept;

  ~Surface();

  VkSurfaceKHR getVkSurface() const;

private:
  VkSurfaceKHR _surface = VK_NULL_HANDLE;
  const Instance* _instance = nullptr;
};
