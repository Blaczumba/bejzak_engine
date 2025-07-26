#pragma once

#include "entity.h"

#include <vector>

class EntityManager {
private:
    std::vector<Entity> _availableEntities;

public:
    EntityManager();

    Entity createEntity();
    void destroyEntity(Entity entity);
};