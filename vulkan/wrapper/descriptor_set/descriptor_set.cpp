#include "descriptor_set.h"

#include <algorithm>
#include <iterator>

#include "descriptor_pool.h"
#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/descriptor_set/descriptor_pool.h"
#include "vulkan/wrapper/descriptor_set/lib.h"

DescriptorSet::DescriptorSet(VkDescriptorSet descriptorSet,
                             const std::shared_ptr<const DescriptorPool>& descriptorPool) noexcept
  : _descriptorSet(descriptorSet), _descriptorPool(descriptorPool) {}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet) noexcept
  : _descriptorSet(descriptorSet._descriptorSet),
    _descriptorPool(std::move(descriptorSet._descriptorPool)) {}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& descriptorSet) noexcept {
  if (this == &descriptorSet) {
    return *this;
  }
  _descriptorSet = descriptorSet._descriptorSet;
  _descriptorPool = std::move(descriptorSet._descriptorPool);

  return *this;
}

DescriptorSet DescriptorSet::create(
    const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout) {
  VkDescriptorSet descriptorSet;
  internal::allocateDescriptorSets(*descriptorPool, std::span{&layout, 1}, &descriptorSet);
  return DescriptorSet(descriptorSet, descriptorPool);
}

std::vector<DescriptorSet> DescriptorSet::create(
    const std::shared_ptr<const DescriptorPool>& descriptorPool,
    std::span<const VkDescriptorSetLayout> layouts) {
  lib::Buffer<VkDescriptorSet> vkDescriptorSets(layouts.size());
  internal::allocateDescriptorSets(*descriptorPool, layouts, vkDescriptorSets.data());
  std::vector<DescriptorSet> descriptorSets;
  descriptorSets.reserve(descriptorSets.size());
  std::transform(vkDescriptorSets.cbegin(), vkDescriptorSets.cend(),
                 std::back_inserter(descriptorSets), [&](VkDescriptorSet descriptorSet) {
                   return DescriptorSet(descriptorSet, descriptorPool);
                 });
  return descriptorSets;
}

VkDescriptorSet DescriptorSet::getVkDescriptorSet() const noexcept {
  return _descriptorSet;
}

const DescriptorPool& DescriptorSet::getDescriptorPool() const {
  return *_descriptorPool;
}
