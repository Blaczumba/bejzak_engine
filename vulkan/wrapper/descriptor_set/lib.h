#pragma once

#include <expected>
#include <span>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"

class DescriptorPool;

namespace internal {

void allocateDescriptorSets(
    const DescriptorPool& descriptorPool, std::span<const VkDescriptorSetLayout> layouts,
    VkDescriptorSet* descriptorSets);

}  // namespace internal
