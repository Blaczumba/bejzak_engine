#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/instance/instance.h"

class DebugMessenger {
  DebugMessenger(const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger) noexcept;

public:
  DebugMessenger() noexcept = default;

  DebugMessenger(DebugMessenger&& debugMessenger) noexcept;

  DebugMessenger& operator=(DebugMessenger&& debugMessenger) noexcept;

  static DebugMessenger create(
      const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  static std::unique_ptr<DebugMessenger> createPtr(
      const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  ~DebugMessenger();

private:
  VkDebugUtilsMessengerEXT _debugUtilsMessenger = VK_NULL_HANDLE;

  const Instance* _instance = nullptr;
};
