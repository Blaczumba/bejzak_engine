#pragma once

#include <vector>

#include "common/entity_component_system/registry/registry.h"
#include "system.h"

class MovementSystem : public System {
  Registry* registry;
  std::vector<Entity> _entities;

public:
  MovementSystem(Registry* reg);

  void update(float deltaTime) override;
};
