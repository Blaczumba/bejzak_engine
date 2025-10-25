#include "object.h"

Object::Object(std::string_view name, Entity entity)
  : _entity(entity), _name(name), _parent(nullptr) {}

void Object::SetParent(Object* newParent) {
  if (_parent) {
    std::vector<Object*>& parentChildren = _parent->_children;
    auto it = std::find(parentChildren.cbegin(), parentChildren.cend(), this);
    if (it != parentChildren.cend()) {
      parentChildren.erase(it);
    }
  }
  _parent = newParent;
  if (newParent) {
    newParent->_children.push_back(this);
  }
}

Entity Object::getEntity() const {
  return _entity;
}
