#pragma once

#include <functional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/sampler/sampler.h"

template <class T>
inline void hashCombine(const T& v, size_t& seed) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct PipelineLayoutKey {
  const std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  const std::vector<VkPushConstantRange> pushConstants;
  const VkPipelineLayoutCreateFlags createFlags = 0;

  bool operator==(const PipelineLayoutKey& other) const {
    if (descriptorSetLayouts.size() != other.descriptorSetLayouts.size()
        || pushConstants.size() != other.pushConstants.size()) {
      return false;
    }

    return memcmp(descriptorSetLayouts.data(), other.descriptorSetLayouts.data(),
                  descriptorSetLayouts.size() * sizeof(VkDescriptorSetLayout))
               == 0
           && memcmp(pushConstants.data(), other.pushConstants.data(),
                     pushConstants.size() * sizeof(VkPushConstantRange))
                  == 0
           && createFlags == other.createFlags;
  }
};

struct PipelineLayoutHasher {
  size_t operator()(const PipelineLayoutKey& key) const {
    size_t seed = 0;
    for (VkDescriptorSetLayout layout : key.descriptorSetLayouts) {
      hashCombine(reinterpret_cast<uint64_t>(layout), seed);
    }

    for (const VkPushConstantRange& pc : key.pushConstants) {
      hashCombine(pc.size, seed);
      hashCombine(pc.offset, seed);
      hashCombine(static_cast<uint32_t>(pc.stageFlags), seed);
    }

    hashCombine(static_cast<uint32_t>(key.createFlags), seed);
    return seed;
  }
};

struct SamplerHasher {
  size_t operator()(const SamplerMetadata& key) const {
    size_t seed = 0;

    // Helper lambda to normalize float zero (-0.0f vs +0.0f) before hashing
    auto hashFloat = [&seed](float val) {
      float normalized = (val == 0.0f) ? 0.0f : val;
      hashCombine(normalized, seed);
    };

    // Helper lambda to hash optional fields
    auto hashOptionalFloat = [&seed, &hashFloat](const std::optional<float>& opt) {
      hashCombine(opt.has_value(), seed);
      if (opt.has_value()) {
        hashFloat(*opt);
      }
    };

    hashCombine(static_cast<uint32_t>(key.flags), seed);
    hashCombine(static_cast<uint32_t>(key.magFilter), seed);
    hashCombine(static_cast<uint32_t>(key.minFilter), seed);
    hashCombine(static_cast<uint32_t>(key.mipmapMode), seed);
    hashCombine(static_cast<uint32_t>(key.addressModeU), seed);
    hashCombine(static_cast<uint32_t>(key.addressModeV), seed);
    hashCombine(static_cast<uint32_t>(key.addressModeW), seed);

    hashFloat(key.mipLodBias);

    // Optional maxAnisotropy
    hashOptionalFloat(key.maxAnisotropy);

    // Optional compareOp
    hashCombine(key.compareOp.has_value(), seed);
    if (key.compareOp.has_value()) {
      hashCombine(static_cast<uint32_t>(*key.compareOp), seed);
    }

    hashFloat(key.minLod);
    hashFloat(key.maxLod);
    hashCombine(static_cast<uint32_t>(key.borderColor), seed);
    hashCombine(static_cast<uint32_t>(key.unnormalizedCoordinates), seed);

    return seed;
  }
};
