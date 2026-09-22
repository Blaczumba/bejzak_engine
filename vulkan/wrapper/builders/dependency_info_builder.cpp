#include "vulkan/wrapper/builders/dependency_info_builder.h"

#include <algorithm>
#include <iterator>
#include <vulkan/vulkan.h>

ImageMemoryBarrierBuilder& DependencyInfoBuilder::addImageMemoryBarrier() {
  return _imageBarrierBuilders.emplace_back();
}

DependencyInfoBuilder& DependencyInfoBuilder::clearBuilders() {
  _imageBarrierBuilders.clear();
  return *this;
}

VkDependencyInfo DependencyInfoBuilder::build(VkDependencyFlags flags) {
  _imageMemoryBarriers.clear();
  _imageMemoryBarriers.reserve(_imageBarrierBuilders.size());
  std::transform(
      std::cbegin(_imageBarrierBuilders), std::cend(_imageBarrierBuilders),
      std::back_inserter(_imageMemoryBarriers), [](const ImageMemoryBarrierBuilder& builder) {
        return builder.build();
      });

  return VkDependencyInfo{
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .pNext = _pNext,
    .dependencyFlags = flags,
    .pMemoryBarriers = nullptr,
    .bufferMemoryBarrierCount = 0,
    .pBufferMemoryBarriers = nullptr,
    .imageMemoryBarrierCount = static_cast<uint32_t>(_imageMemoryBarriers.size()),
    .pImageMemoryBarriers = _imageMemoryBarriers.data()};
}
