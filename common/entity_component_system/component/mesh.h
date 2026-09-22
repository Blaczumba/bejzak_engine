#pragma once

#include <memory>
// TODO: remove vulkan dependency
#include <vulkan/vulkan.h>

#include "common/entity_component_system/entity/entity.h"
#include "common/ref/ref.h"
#include "common/util/geometry.h"

class MeshComponent {
  static constexpr ComponentType componentID = 2;

public:
  common::Ref<common::RefType::Buffer> vertexBufferHandle;
  common::Ref<common::RefType::Buffer> indexBufferHandle;
  common::Ref<common::RefType::Buffer> vertexBufferPrimitiveHandle;
  AABB aabb;
  VkIndexType indexType;

  static constexpr std::enable_if_t < componentID<MAX_COMPONENTS, ComponentType> getComponentID() {
    return componentID;
  }
};
