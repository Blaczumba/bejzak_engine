#pragma once

#include <deque>
#include <vector>

#include "vulkan/wrapper/builders/image_memory_barrier_builder.h"

/*Builder pattern for safe construction of VkDependencyInfo.
The class aggregates barrier builders because each builder
might define a pNext chain with elements which need to live
until the dependency info is consumed.*/
class DependencyInfoBuilder {
public:
  ImageMemoryBarrierBuilder& addImageMemoryBarrier();

  DependencyInfoBuilder& clearBuilders();

  VkDependencyInfo build(VkDependencyFlags flags = {});

private:
  // Use deque for reference stability.
  std::deque<ImageMemoryBarrierBuilder> _imageBarrierBuilders;
  std::vector<VkImageMemoryBarrier2> _imageMemoryBarriers;
  // And other types of barriers below...

  void* _pNext = nullptr;
};
