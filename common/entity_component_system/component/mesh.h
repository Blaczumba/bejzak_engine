#pragma once

#include <memory>

#include "common/entity_component_system/entity/entity.h"
#include "common/util/geometry.h"
#include "vulkan/wrapper/memory_objects/buffer.h" // TODO: do not use vulkan specific things in this directory

class MeshComponent {
  static constexpr ComponentType componentID = 2;

public:
  Buffer vertexBuffer;
  Buffer indexBuffer;
  Buffer vertexBufferPrimitive;
  AABB aabb;
  VkIndexType indexType;

  static constexpr std::enable_if_t < componentID<MAX_COMPONENTS, ComponentType> getComponentID() {
    return componentID;
  }
};
