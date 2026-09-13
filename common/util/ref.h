#pragma once

#include <cstdint>
#include <tuple>

namespace common {

enum class RefType : uint8_t {
  Undefined,
  Buffer,
  Image,
  Sampler,
  VirtualAllocation
};

template <RefType type>
class Ref {
  struct VTable {
    void (*incrementRefCount)(void* referenceCounter, uint32_t handle);
    void (*decrementRefCount)(void* referenceCounter, uint32_t handle);
  };

  // One VTable instance per (ReferenceCounter, Handle) pair, shared by all Refs
  // built from that pair. Returned by pointer so every Ref stores just 8 bytes.
  template <typename ReferenceCounter, typename Handle>
  static const VTable* vtableFor() {
    static constexpr VTable table{
      [](void* referenceCounter, uint32_t handle) {
        static_cast<ReferenceCounter*>(referenceCounter)
            ->incrementRefCount(static_cast<Handle>(handle));
      },
      [](void* referenceCounter, uint32_t handle) {
        static_cast<ReferenceCounter*>(referenceCounter)
            ->decrementRefCount(static_cast<Handle>(handle));
      },
    };
    return &table;
  }

public:
  Ref() noexcept = default;

  template <typename ReferenceCounter, typename Handle>
  Ref(ReferenceCounter* referenceCounter, Handle handle)
    : _referenceCounter(referenceCounter), _vtable(vtableFor<ReferenceCounter, Handle>()),
      _handle(static_cast<uint32_t>(*handle)) {
    if (referenceCounter) {
      _vtable->incrementRefCount(_referenceCounter, _handle);
    }
  }

  Ref(const Ref& other)
    : _referenceCounter(other._referenceCounter), _vtable(other._vtable), _handle(other._handle) {
    if (_referenceCounter != nullptr) {
      _vtable->incrementRefCount(_referenceCounter, _handle);
    }
  }

  Ref(Ref&& other) noexcept
    : _referenceCounter(std::exchange(other._referenceCounter, nullptr)), _vtable(other._vtable),
      _handle(other._handle) {}

  Ref& operator=(const Ref& other) {
    if (this == &other) {
      return *this;
    }
    if (_referenceCounter != nullptr) {
      _vtable->decrementRefCount(_referenceCounter, _handle);
    }
    _referenceCounter = other._referenceCounter;
    if (_referenceCounter != nullptr) {
      _handle = other._handle;
      _vtable = other._vtable;
      _vtable->incrementRefCount(_referenceCounter, _handle);
    }
    return *this;
  }

  Ref& operator=(Ref&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    if (_referenceCounter != nullptr) {
      _vtable->decrementRefCount(_referenceCounter, _handle);
    }
    _referenceCounter = std::exchange(other._referenceCounter, nullptr);
    _vtable = other._vtable;
    _handle = other._handle;
    return *this;
  }

  ~Ref() {
    if (_referenceCounter != nullptr) {
      _vtable->decrementRefCount(_referenceCounter, _handle);
    }
  }

  // Do not use it directly.
  template <typename ReferenceCounter, typename Handle>
  static Ref adopt(ReferenceCounter* referenceCounter, Handle handle) noexcept {
    Ref ref;
    if (referenceCounter) {
      ref._referenceCounter = referenceCounter;
      ref._vtable = vtableFor<ReferenceCounter, Handle>();
      ref._handle = static_cast<uint32_t>(*handle);
    }
    return ref;
  }

  std::tuple<void*, uint32_t> release() noexcept {
    return std::make_tuple(std::exchange(_referenceCounter, nullptr), _handle);
  }

  void* getReferenceCounter() const noexcept {
    return _referenceCounter;
  }

  uint32_t getHandle() const noexcept {
    return _handle;
  }

private:
  void* _referenceCounter = nullptr;
  const VTable* _vtable;
  uint32_t _handle;
};

}  // namespace common
