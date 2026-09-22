#pragma once

#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class Fence {
  Fence(const LogicalDevice& logicalDevice, VkFence fence);

public:
  Fence() noexcept = default;

  static Fence create(const LogicalDevice& logicalDevice, const VkFenceCreateInfo& createInfo);

  Fence(Fence&& other) noexcept;

  Fence& operator=(Fence&& other) noexcept;

  ~Fence();

  VkResult wait(uint64_t timeout = UINT64_MAX) const;

  VkResult reset() const;

  VkFence getVkFence() const noexcept;

private:
  VkFence _fence = VK_NULL_HANDLE;
  const LogicalDevice* _logicalDevice = nullptr;

  void destroy();
};

class FenceBuilder {
public:
  Fence build(const LogicalDevice& logicalDevice, VkFenceCreateFlags flags = {});

private:
  VkFenceCreateFlags _flags;

  void* _pNext = nullptr;
};
