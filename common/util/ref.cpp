#include "ref.h"

#include <utility>
#include <tuple>
#include <cstdint>

namespace common {

Ref::Ref(const Ref& other)
  : _referenceCounter(other._referenceCounter), _vtable(other._vtable), _handle(other._handle), _type(other._type) {
  if (_referenceCounter != nullptr) {
    _vtable->incrementRefCount(_referenceCounter, _handle);
  }
}

Ref::Ref(Ref&& other) noexcept
  : _referenceCounter(std::exchange(other._referenceCounter, nullptr)), _vtable(other._vtable),
    _handle(other._handle), _type(std::exchange(other._type, Type::UNDEFINED)) {}

Ref& Ref::operator=(const Ref& other) {
  if (this == &other) {
    return *this;
  }
  if (_referenceCounter != nullptr) {
    _vtable->decrementRefCount(_referenceCounter, _handle);
  }
  _referenceCounter = other._referenceCounter;
  _vtable = other._vtable;
  _handle = other._handle;
  _type = other._type;
  if (_referenceCounter != nullptr) {
    _vtable->incrementRefCount(_referenceCounter, _handle);
  }
  return *this;
}

Ref& Ref::operator=(Ref&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (_referenceCounter != nullptr) {
    _vtable->decrementRefCount(_referenceCounter, _handle);
  }
  _referenceCounter = std::exchange(other._referenceCounter, nullptr);
  _vtable = other._vtable;
  _handle = other._handle;
  _type = std::exchange(other._type, Type::UNDEFINED);
  return *this;
}

Ref::~Ref() {
  if (_referenceCounter != nullptr) {
    _vtable->decrementRefCount(_referenceCounter, _handle);
  }
}

std::tuple<void*, uint32_t> Ref::release() noexcept {
  _type = Type::UNDEFINED;
  return std::make_tuple(
      std::exchange(_referenceCounter, nullptr), _handle);
}

void* Ref::getReferenceCounter() const noexcept {
  return _referenceCounter;
}

uint32_t Ref::getHandle() const noexcept {
  return _handle;
}

Ref::Type Ref::getType() const noexcept {
  return _type;
}

}  // namespace common
