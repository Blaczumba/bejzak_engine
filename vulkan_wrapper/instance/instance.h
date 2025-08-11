#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"

class Instance {
  VkInstance _instance = VK_NULL_HANDLE;

  Instance(VkInstance instance);

public:
  Instance() = default;

  Instance(Instance&& other) noexcept;

  Instance& operator=(Instance&& other) noexcept;

  ~Instance();

  static ErrorOr<Instance> create(
      std::string_view engineName, std::span<const char* const> requiredExtensions,
      PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  static ErrorOr<Instance> createFromInitialized(VkInstance instance);

  VkInstance getVkInstance() const;

  ErrorOr<lib::Buffer<VkPhysicalDevice>> getAvailablePhysicalDevices() const;
};
