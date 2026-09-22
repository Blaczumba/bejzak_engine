#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/descriptor_set/lib.h"

class DescriptorSet {
  DescriptorSet(VkDescriptorSet descriptorSet,
                const std::shared_ptr<const DescriptorPool>& descriptorPool) noexcept;

public:
  DescriptorSet() noexcept = default;

  DescriptorSet(DescriptorSet&& descriptorSet) noexcept;

  DescriptorSet& operator=(DescriptorSet&& DescriptorSet) noexcept;

  ~DescriptorSet() = default;

  static DescriptorSet create(
      const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout);

  template <std::size_t COUNT>
  static std::array<DescriptorSet, COUNT> create(
      const std::shared_ptr<const DescriptorPool>& descriptorPool,
      std::span<const VkDescriptorSetLayout> layouts);

  static std::vector<DescriptorSet> create(
      const std::shared_ptr<const DescriptorPool>& descriptorPool,
      std::span<const VkDescriptorSetLayout> layouts);

  VkDescriptorSet getVkDescriptorSet() const noexcept;

  const DescriptorPool& getDescriptorPool() const;

private:
  VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

  std::shared_ptr<const DescriptorPool> _descriptorPool;
};

template <std::size_t COUNT>
std::array<DescriptorSet, COUNT> DescriptorSet::create(
    const std::shared_ptr<const DescriptorPool>& descriptorPool,
    std::span<const VkDescriptorSetLayout> layouts) {
  std::array<VkDescriptorSet, COUNT> descriptorSets;
  internal::allocateDescriptorSets(*descriptorPool, layouts, descriptorSets.data());
  std::array<DescriptorSet, COUNT> descSets;
  std::transform(std::cbegin(descriptorSets), std::cend(descriptorSets), descSets.begin(),
                 [&descriptorPool](VkDescriptorSet descriptorSet) {
                   return DescriptorSet(descriptorSet, descriptorPool);
                 });
  return descSets;
}
