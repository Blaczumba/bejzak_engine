#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "common/entity_component_system/entity/entity.h"

class TransformComponent {
  static constexpr ComponentType componentID = 4;

public:
  glm::mat4 model;

  static constexpr std::enable_if_t < componentID<MAX_COMPONENTS, ComponentType> getComponentID() {
    return componentID;
  }
};
