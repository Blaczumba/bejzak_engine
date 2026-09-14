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

// For performance reasons, every graphics wrapper defines it on its own.
template <RefType type>
struct RefTraits;

template <RefType type>
class Ref {
public:
  Ref() noexcept = default;

  template <typename ReferenceCounter, typename Handle>
  Ref(ReferenceCounter* referenceCounter, Handle handle)
    : _referenceCounter(referenceCounter), _handle(static_cast<uint32_t>(*handle)) {
    if (referenceCounter) {
      RefTraits<type>::increment(_referenceCounter, _handle);
    }
  }

  Ref(const Ref& other) : _referenceCounter(other._referenceCounter), _handle(other._handle) {
    if (_referenceCounter != nullptr) {
      RefTraits<type>::increment(_referenceCounter, _handle);
    }
  }

  Ref(Ref&& other) noexcept
    : _referenceCounter(std::exchange(other._referenceCounter, nullptr)), _handle(other._handle) {}

  Ref& operator=(const Ref& other) {
    if (this == &other) [[unlikely]] {
      return *this;
    }
    if (_referenceCounter != nullptr) {
      RefTraits<type>::decrement(_referenceCounter, _handle);
    }
    _referenceCounter = other._referenceCounter;
    if (_referenceCounter != nullptr) {
      _handle = other._handle;
      RefTraits<type>::increment(_referenceCounter, _handle);
    }
    return *this;
  }

  Ref& operator=(Ref&& other) noexcept {
    if (this == &other) [[unlikely]] {
      return *this;
    }
    if (_referenceCounter != nullptr) {
      RefTraits<type>::decrement(_referenceCounter, _handle);
    }
    _referenceCounter = std::exchange(other._referenceCounter, nullptr);
    _handle = other._handle;
    return *this;
  }

  ~Ref() {
    if (_referenceCounter != nullptr) {
      RefTraits<type>::decrement(_referenceCounter, _handle);
    }
  }

  // Do not use it directly.
  template <typename ReferenceCounter, typename Handle>
  static Ref adopt(ReferenceCounter* referenceCounter, Handle handle) noexcept {
    Ref ref;
    if (referenceCounter) {
      ref._referenceCounter = referenceCounter;
      ref._handle = static_cast<uint32_t>(*handle);
    }
    return ref;
  }

  // Do not use it directly.
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
  uint32_t _handle;
};

}  // namespace common
