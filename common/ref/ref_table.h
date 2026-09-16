#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace common {

template <typename Handle, std::size_t N>
class RefTable {
  // Only ResourceManager is erased (via void* + vtable). Handle is a template
  // parameter, so it's stored and passed through as-is. The space win comes
  // from sharing the manager and vtable pointers across all N handles rather
  // than duplicating them in N separate Refs.
  struct Vtable {
    void (*incrementRefCount)(void* resourceManager, Handle handle);
    void (*decrementRefCount)(void* resourceManager, Handle handle);
  };

  template <typename ResourceManager>
  static const Vtable* vtableFor() {
    static constexpr Vtable table{
      [](void* resourceManager, Handle handle) {
        static_cast<ResourceManager*>(resourceManager)->incrementRefCount(handle);
      },
      [](void* resourceManager, Handle handle) {
        static_cast<ResourceManager*>(resourceManager)->decrementRefCount(handle);
      },
    };
    return &table;
  }

public:
  RefTable() noexcept = default;

  template <typename ResourceManager>
  RefTable(ResourceManager* resourceManager, std::span<const Handle, N> handles);

  RefTable(const RefTable& other);

  RefTable(RefTable&& other) noexcept;

  RefTable& operator=(const RefTable& other);

  RefTable& operator=(RefTable&& other) noexcept;

  ~RefTable();

  std::span<const Handle, N> handles() const noexcept {
    return _handles;
  }

  template <std::size_t Index>
  Handle getHandle() const noexcept {
    static_assert(Index < N, "RefTable: Index out of bounds.");
    return _handles[Index];
  }

  Handle getHandle(std::size_t index) const {
    return _handles[index];
  }

private:
  void incrementAll() noexcept;

  void decrementAll() noexcept;

  void* _resourceManager = nullptr;
  const Vtable* _vtable;
  std::array<Handle, N> _handles;
};

template <typename Handle, std::size_t N>
template <typename ResourceManager>
RefTable<Handle, N>::RefTable(ResourceManager* resourceManager, std::span<const Handle, N> handles)
  : _resourceManager(resourceManager), _vtable(vtableFor<ResourceManager>()) {
  std::transform(std::cbegin(handles), std::cend(handles), _handles.begin(), [](Handle handle) {
    return handle;
  });
  incrementAll();
}

template <typename Handle, std::size_t N>
RefTable<Handle, N>::RefTable(const RefTable& other)
  : _resourceManager(other._resourceManager), _vtable(other._vtable), _handles(other._handles) {
  incrementAll();
}

template <typename Handle, std::size_t N>
RefTable<Handle, N>::RefTable(RefTable&& other) noexcept
  : _resourceManager(std::exchange(other._resourceManager, nullptr)), _vtable(other._vtable),
    _handles(other._handles) {}

template <typename Handle, std::size_t N>
RefTable<Handle, N>& RefTable<Handle, N>::operator=(const RefTable& other) {
  if (this == &other) {
    return *this;
  }
  decrementAll();
  _resourceManager = other._resourceManager;
  _vtable = other._vtable;
  _handles = other._handles;
  incrementAll();
  return *this;
}

template <typename Handle, std::size_t N>
RefTable<Handle, N>& RefTable<Handle, N>::operator=(RefTable&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  decrementAll();
  _resourceManager = std::exchange(other._resourceManager, nullptr);
  _vtable = other._vtable;
  _handles = other._handles;
  return *this;
}

template <typename Handle, std::size_t N>
RefTable<Handle, N>::~RefTable() {
  decrementAll();
}

template <typename Handle, std::size_t N>
void RefTable<Handle, N>::incrementAll() noexcept {
  if (_resourceManager) {
    for (const Handle& handle : _handles) {
      _vtable->incrementRefCount(_resourceManager, handle);
    }
  }
}

template <typename Handle, std::size_t N>
void RefTable<Handle, N>::decrementAll() noexcept {
  if (_resourceManager) {
    for (const Handle& handle : _handles) {
      _vtable->decrementRefCount(_resourceManager, handle);
    }
  }
}

}  // namespace common
