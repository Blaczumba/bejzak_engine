#pragma once

#include <cstdint>

#include "vulkan/resource_manager/handle.h"

template <typename Resource>
class ReferenceCounter {
public:
  virtual void incrementRefCount(HandleFor<Resource> handle) = 0;

  virtual void decrementRefCount(HandleFor<Resource> handle) = 0;
};
