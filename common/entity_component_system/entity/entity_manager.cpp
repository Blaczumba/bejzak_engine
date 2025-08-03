#include "entity_manager.h"

#include <algorithm>
#include <iterator>

EntityManager::EntityManager() {
    _availableEntities.reserve(MAX_ENTITIES);
    std::generate_n(std::back_inserter(_availableEntities), MAX_ENTITIES, [val = MAX_ENTITIES]() mutable { return --val; });
}

Entity EntityManager::createEntity() {
    const Entity entity = _availableEntities.back();
    _availableEntities.pop_back();
    return entity;
}

void EntityManager::destroyEntity(Entity entity) {
    _availableEntities.push_back(entity);
}
