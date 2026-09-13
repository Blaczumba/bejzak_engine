#pragma once

#include <cstdint>

#include "vulkan/resource_manager/handle.h"

template <typename Resource>
class ReferenceCounter {
public:
  virtual void incrementRefCount(HandleFor<Resource> handle) = 0;

  virtual void decrementRefCount(HandleFor<Resource> handle) = 0;

  // Must be called when related Ref<Resource> is still alive.
  virtual VulkanObjectFor<Resource> getVkResource(HandleFor<Resource> handle) const = 0;

  // Must be called when related Ref<Resource> is still alive.
  virtual const MetadataFor<Resource>& getMetadata(HandleFor<Resource> handle) const = 0;
};
