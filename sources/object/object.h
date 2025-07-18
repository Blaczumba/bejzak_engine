#pragma once

#include "descriptor_set/descriptor_set.h"
#include "entity_component_system/entity/entity.h"
#include "primitives/geometry.h"

#include <algorithm>
#include <memory>
#include <string_view>

#include <glm/glm.hpp>

class Object {
    Entity _entity;
    std::string_view _name;
    Object* _parent;
    std::vector<Object*> _children;

public:
    Object(std::string_view name, Entity entity);
    void SetParent(Object* newParent);
    Entity getEntity() const;
};
