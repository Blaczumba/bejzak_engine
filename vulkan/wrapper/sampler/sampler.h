#pragma once

#include <optional>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class Sampler {
  Sampler(const LogicalDevice& logicalDevice, VkSampler sampler) noexcept;

public:
  Sampler() noexcept = default;

  static Sampler create(const LogicalDevice& logicalDevice, const VkSamplerCreateInfo& samplerInfo);

  Sampler(Sampler&& other) noexcept;

  Sampler& operator=(Sampler&& other) noexcept;

  ~Sampler();

  VkSampler getVkSampler() const noexcept;

  VkSampler getVkResource() const noexcept;

private:
  VkSampler _sampler = VK_NULL_HANDLE;
  const LogicalDevice* _logicalDevice = nullptr;

  void destroy();
};

struct SamplerMetadata {
  VkSamplerCreateFlags flags;
  VkFilter magFilter;
  VkFilter minFilter;
  VkSamplerMipmapMode mipmapMode;
  VkSamplerAddressMode addressModeU;
  VkSamplerAddressMode addressModeV;
  VkSamplerAddressMode addressModeW;
  float mipLodBias;
  std::optional<float> maxAnisotropy;
  std::optional<VkCompareOp> compareOp;
  float minLod;
  float maxLod;
  VkBorderColor borderColor;
  VkBool32 unnormalizedCoordinates;
  // Other std::optional fields representing pNext metadata.

  bool operator==(const SamplerMetadata&) const = default;
};

class SamplerBuilder {
public:
  SamplerBuilder() noexcept = default;

  SamplerBuilder&& withMinMagFilter(VkFilter minFiler, VkFilter magFilter) && noexcept;

  SamplerBuilder&& withMipmapMode(VkSamplerMipmapMode mode) && noexcept;

  SamplerBuilder&& withAddressMode(
      VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
      VkSamplerAddressMode addressModeW) && noexcept;

  SamplerBuilder&& withMipLodBias(float mipLodBias) && noexcept;

  SamplerBuilder&& withMaxAnisotropy(float maxAnisotropy) && noexcept;

  SamplerBuilder&& withMaxAnisotropy(std::optional<float> maxAnisotropy) && noexcept;

  SamplerBuilder&& withCompareOp(VkCompareOp compareOp) && noexcept;

  SamplerBuilder&& withCompareOp(std::optional<VkCompareOp> compareOp) && noexcept;

  SamplerBuilder&& withLodRange(float minLod, float maxLod) && noexcept;

  SamplerBuilder&& withBorderColor(VkBorderColor borderColor) && noexcept;

  SamplerBuilder&& withUnnormalizedCoordinates(VkBool32 unnormalizedCoordinates) && noexcept;

  SamplerBuilder&& withFlags(VkSamplerCreateFlags flags) && noexcept;

  std::tuple<Sampler, SamplerMetadata> buildSamplerWithMetadata(
      const LogicalDevice& logicalDevice) const;

  SamplerMetadata buildMetadata() const noexcept;

  Sampler buildSampler(const LogicalDevice& logicalDevice) const;

private:
  VkSamplerCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 0.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_NEVER,
    .minLod = 0.0f,
    .maxLod = VK_LOD_CLAMP_NONE,
    .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
  };

  void* _pNext = nullptr;
};
