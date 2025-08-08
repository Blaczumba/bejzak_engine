#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "common/entity_component_system/entity/entity.h"

class VelocityComponent {
  static constexpr ComponentType componentID = 5;

public:
  float dx, dy;

  static constexpr std::enable_if_t < componentID<MAX_COMPONENTS, ComponentType> getComponentID() {
    return componentID;
  }
};
