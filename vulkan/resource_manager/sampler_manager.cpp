#include "sampler_manager.h"

#include "vulkan/resource_manager/reference_counter_with_hashing.h"
#include "vulkan/wrapper/sampler/sampler.h"

std::unique_ptr<SamplerManager> SamplerManager::create() {
  return std::unique_ptr<SamplerManager>(new SamplerManager());
}

Ref<Sampler> SamplerManager::getOrCreateSampler(
    const LogicalDevice& logicalDevice, const SamplerMetadata& metadata) {
  return getOrCreateResource(
      [](const LogicalDevice& logicalDevice, const SamplerMetadata& metadata) {
        return SamplerBuilder()
            .withFlags(metadata.flags)
            .withMinMagFilter(metadata.minFilter, metadata.magFilter)
            .withMipmapMode(metadata.mipmapMode)
            .withAddressMode(metadata.addressModeU, metadata.addressModeV, metadata.addressModeW)
            .withMipLodBias(metadata.mipLodBias)
            .withMaxAnisotropy(metadata.maxAnisotropy)
            .withCompareOp(metadata.compareOp)
            .withLodRange(metadata.minLod, metadata.maxLod)
            .withBorderColor(metadata.borderColor)
            .withUnnormalizedCoordinates(metadata.unnormalizedCoordinates)
            .buildSampler(logicalDevice);
      },
      logicalDevice, metadata);
}
