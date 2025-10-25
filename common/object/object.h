#pragma once

#include <glm/glm.hpp>
#include <string_view>
#include <vector>

#include "common/entity_component_system/entity/entity.h"
#include "common/util/geometry.h"

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
