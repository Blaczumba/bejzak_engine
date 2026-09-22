#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class Semaphore {
  Semaphore(const LogicalDevice& logicalDevice, VkSemaphore semaphore);

public:
  Semaphore() noexcept = default;

  static Semaphore create(
      const LogicalDevice& logicalDevice, const VkSemaphoreCreateInfo& createInfo);

  Semaphore(Semaphore&& other) noexcept;

  Semaphore& operator=(Semaphore&& other) noexcept;

  ~Semaphore();

  VkSemaphore getVkSemaphore() const noexcept;

private:
  VkSemaphore _semaphore = VK_NULL_HANDLE;
  const LogicalDevice* _logicalDevice = nullptr;

  void destroy();
};

class SemaphoreBuilder {
public:
  SemaphoreBuilder& withType(VkSemaphoreType type, uint64_t initialValue = 0) noexcept;

  Semaphore build(const LogicalDevice& logicalDevice, VkSemaphoreCreateFlags flags = {});

private:
  VkSemaphoreCreateFlags _flags;

  std::optional<VkSemaphoreTypeCreateInfo> _typeInfo;

  void* _pNext = nullptr;
};
