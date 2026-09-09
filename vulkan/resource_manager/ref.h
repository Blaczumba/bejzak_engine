#pragma once

#include <utility>

#include "common/util/engine_exception.h"
#include "common/util/ref.h"
#include "vulkan/resource_manager/handle.h"
#include "vulkan/resource_manager/reference_counter.h"

namespace {

template <typename Resource>
constexpr common::Ref::Type deduceResourceType() {
  return common::Ref::Type::UNDEFINED;
}

template <>
constexpr common::Ref::Type deduceResourceType<Buffer>() {
  return common::Ref::Type::BUFFER;
}

template <>
constexpr common::Ref::Type deduceResourceType<Image>() {
  return common::Ref::Type::IMAGE;
}

}  // namespace

template <typename Resource>
class Ref {
public:
  Ref() noexcept = default;

  Ref(ReferenceCounter<Resource>& counter, HandleFor<Resource> handle)
    : _counter(&counter), _handle(handle) {
    _counter->incrementRefCount(_handle);
  }

  Ref(const Ref& other) : _counter(other._counter), _handle(other._handle) {
    if (_counter != nullptr) {
      _counter->incrementRefCount(_handle);
    }
  }

  Ref(const common::Ref& other)
    : _counter(static_cast<ReferenceCounter<Resource>*>(other.getReferenceCounter())) {
    if (deduceResourceType<Resource>() != other.getType()) {
      throw EngineException("Type mismatch in Ref constructor");
    }
    if (_counter != nullptr) {
      _counter->incrementRefCount(_handle);
      _handle = static_cast<HandleFor<Resource>>(other.getHandle());
    }
  }

  Ref(Ref&& other) noexcept
    : _counter(std::exchange(other._counter, nullptr)), _handle(other._handle) {}

  Ref(common::Ref&& other) {
    if (deduceResourceType<Resource>() != other.getType()) {
      throw EngineException("Type mismatch in Ref = operator");
    }
    const auto [referenceCounter, handle] = other.release();
    if (referenceCounter != nullptr) {
      _counter = static_cast<ReferenceCounter<Resource>*>(referenceCounter);
      _handle = static_cast<HandleFor<Resource>>(handle);
    }
  }

  Ref& operator=(const Ref& other) {
    if (this == &other) {
      return *this;
    }

    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }

    _counter = other._counter;
    _handle = other._handle;

    if (_counter != nullptr) {
      _counter->incrementRefCount(_handle);
    }
    return *this;
  }

  Ref& operator=(const common::Ref& other) {
    if (deduceResourceType<Resource>() != other.getType()) {
      throw EngineException("Type mismatch in Ref = operator");
    }
    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }
    _counter = static_cast<ReferenceCounter<Resource>*>(other.getReferenceCounter());
    if (_counter != nullptr) {
      _handle = static_cast<HandleFor<Resource>>(other.getHandle());
      _counter->incrementRefCount(static_cast<HandleFor<Resource>>(_handle));
    }
    return *this;
  }

  Ref& operator=(Ref&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }

    _counter = std::exchange(other._counter, nullptr);
    _handle = other._handle;
    return *this;
  }

  Ref& operator=(common::Ref&& other) {
    if (deduceResourceType<Resource>() != other.getType()) {
      throw EngineException("Type mismatch in Ref = operator");
    }
    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }
    const auto [referenceCounter, handle] = other.release();
    _counter = static_cast<ReferenceCounter<Resource>*>(referenceCounter);
    _handle = static_cast<HandleFor<Resource>>(handle);
    return *this;
  }

  operator common::Ref() const& {
    return common::Ref(_counter, _handle, deduceResourceType<Resource>());
  }

  operator common::Ref() && noexcept {
    if (_counter == nullptr) {
      return common::Ref{};
    }
    return common::Ref::adopt<ReferenceCounter<Resource>, HandleFor<Resource>>(
        std::exchange(_counter, nullptr), _handle, deduceResourceType<Resource>());
  }

  ~Ref() {
    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }
  }

  HandleFor<Resource> getHandle() const noexcept {
    return _handle;
  }

  ReferenceCounter<Resource>* getCounter() const noexcept {
    return _counter;
  }

private:
  ReferenceCounter<Resource>* _counter = nullptr;
  HandleFor<Resource> _handle;
};
