#pragma once

#include <cstdint>

#include "vulkan/resource_manager/handle.h"

template <typename Resource>
class ReferenceCounter {
public:
  virtual void incrementRefCount(HandleFor<Resource> handle) = 0;

  virtual void decrementRefCount(HandleFor<Resource> handle) = 0;

  // Must be called when related Ref<Resource> is still alive.
  virtual UnderlyingResourceFor<Resource> getUnderlyingResource(
      HandleFor<Resource> handle) const = 0;

  // Must be called when related Ref<Resource> is still alive.
  virtual const MetadataFor<Resource>& getMetadata(HandleFor<Resource> handle) const = 0;

  // Must be called when related Ref<Resource> is still alive.
  virtual std::tuple<UnderlyingResourceFor<Resource>, const MetadataFor<Resource>&>
  getUnderlyingResourceWithMetadata(HandleFor<Resource> handle) const = 0;
};
