#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan_wrapper/instance/instance.h"

class DebugMessenger {
  VkDebugUtilsMessengerEXT _debugUtilsMessenger;

  const Instance* _instance;

  DebugMessenger(const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger);

public:
  DebugMessenger();

  DebugMessenger(DebugMessenger&& debugMessenger) noexcept;

  DebugMessenger& operator=(DebugMessenger&& debugMessenger) noexcept;

  static ErrorOr<DebugMessenger> create(
      const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  ~DebugMessenger();
};
