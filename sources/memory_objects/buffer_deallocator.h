#pragma once

#include "memory_allocator/memory_allocator.h"
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

struct BufferDeallocator {
    const VkBuffer buffer;

    void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
        allocator.destroyVkBuffer(buffer, allocation);
    }

    void operator()(auto&&, auto&&) {
        // throw std::runtime_error(EngineError::NOT_RECOGNIZED_TYPE);
    }
};