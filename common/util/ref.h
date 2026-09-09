#pragma once

#include <cstdint>
#include <tuple>

namespace common {

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
  enum class Type : uint8_t {
    UNDEFINED,
    BUFFER,
    IMAGE,
    VIRTUAL_ALLOCATION,
  };

  Ref() noexcept = default;

  template <typename ReferenceCounter, typename Handle>
  Ref(ReferenceCounter* referenceCounter, Handle handle, Type type)
    : _referenceCounter(referenceCounter), _vtable(vtableFor<ReferenceCounter, Handle>()),
      _handle(static_cast<uint32_t>(*handle)), _type(type) {
    if (referenceCounter) {
      referenceCounter->incrementRefCout(handle);
    }
  }

  Ref(const Ref& other);

  Ref(Ref&& other) noexcept;

  Ref& operator=(const Ref& other);

  Ref& operator=(Ref&& other) noexcept;

  ~Ref();

  // Do not use it directly.
  template <typename ResourceManager, typename Handle>
  static Ref adopt(ResourceManager* referenceCounter, Handle handle, Type type) noexcept {
    Ref ref;
    if (referenceCounter) {
      ref._referenceCounter = referenceCounter;
      ref._vtable = vtableFor<ResourceManager, Handle>();
      ref._handle = static_cast<uint32_t>(*handle);
      ref._type = type;
    }
    return ref;
  }

  // Do not use it directly.
  std::tuple<void*, uint32_t> release() noexcept;

  void* getReferenceCounter() const noexcept;

  uint32_t getHandle() const noexcept;

  Type getType() const noexcept;

private:
  void* _referenceCounter = nullptr;
  const VTable* _vtable;
  uint32_t _handle;
  Type _type = Type::UNDEFINED;
};

}  // namespace common
