#pragma once

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan_wrapper/memory_allocator/memory_allocator.h"

struct BufferDeallocator {
  const VkBuffer buffer;

  void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
    allocator.destroyVkBuffer(buffer, allocation);
  }

  void operator()(auto&&, auto&&) {}
};
