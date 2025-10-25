#pragma once

#include <vector>

#include "entity.h"

class EntityManager {
private:
  std::vector<Entity> _availableEntities;

public:
  EntityManager();

  Entity createEntity();
  void destroyEntity(Entity entity);
};
