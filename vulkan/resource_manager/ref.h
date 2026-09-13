#pragma once

#include <utility>

#include "common/util/engine_exception.h"
#include "common/util/ref.h"
#include "vulkan/resource_manager/handle.h"
#include "vulkan/resource_manager/reference_counter.h"

template <typename Resource>
class Ref {
public:
  Ref() noexcept = default;

  Ref(ReferenceCounter<Resource>& counter, HandleFor<Resource> handle)
    : _counter(&counter), _handle(handle) {
    _counter->incrementRefCount(_handle);
  }

  Ref(const Ref& other) : _counter(other._counter) {
    if (_counter != nullptr) {
      _handle = other._handle;
      _counter->incrementRefCount(_handle);
    }
  }

  Ref(const common::Ref<ErasedTypeOf<Resource>>& other)
    : _counter(static_cast<ReferenceCounter<Resource>*>(other.getReferenceCounter())) {
    if (_counter != nullptr) {
      _handle = static_cast<HandleFor<Resource>>(other.getHandle());
      _counter->incrementRefCount(_handle);
    }
  }

  Ref(Ref&& other) noexcept
    : _counter(std::exchange(other._counter, nullptr)), _handle(other._handle) {}

  Ref(common::Ref<ErasedTypeOf<Resource>>&& other) noexcept {
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
    if (_counter != nullptr) {
      _handle = other._handle;
      _counter->incrementRefCount(_handle);
    }

    return *this;
  }

  Ref& operator=(const common::Ref<ErasedTypeOf<Resource>>& other) {
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

  Ref& operator=(common::Ref<ErasedTypeOf<Resource>>&& other) noexcept {
    if (_counter != nullptr) {
      _counter->decrementRefCount(_handle);
    }
    const auto [referenceCounter, handle] = other.release();
    _counter = static_cast<ReferenceCounter<Resource>*>(referenceCounter);
    _handle = static_cast<HandleFor<Resource>>(handle);
    return *this;
  }

  operator common::Ref<ErasedTypeOf<Resource>>() const& {
    return common::Ref<ErasedTypeOf<Resource>>(_counter, _handle);
  }

  operator common::Ref<ErasedTypeOf<Resource>>() && noexcept {
    if (_counter == nullptr) {
      return common::Ref<ErasedTypeOf<Resource>>{};
    }
    return common::Ref<ErasedTypeOf<Resource>>::
        template adopt<ReferenceCounter<Resource>, HandleFor<Resource>>(
            std::exchange(_counter, nullptr), _handle);
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

  VulkanObjectFor<Resource> getVkResource() const {
    if (_counter == nullptr) {
      throw EngineException("Attempt to get Vulkan resource from null reference counter");
    }
    return _counter->getVkResource(_handle);
  }

  const MetadataFor<Resource>& getMetadata() const {
    if (_counter == nullptr) {
      throw EngineException("Attempt to get metadata from null reference counter");
    }
    return _counter->getMetadata(_handle);
  }

private:
  ReferenceCounter<Resource>* _counter = nullptr;
  HandleFor<Resource> _handle;
};
