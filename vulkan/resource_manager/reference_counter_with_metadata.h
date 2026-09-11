#pragma once

#include <atomic>
#include <cstdint>
#include <expected>
#include <format>
#include <mutex>
#include <numeric>

#include "lib/sparse/sparse_map.h"
#include "vulkan/resource_manager/handle.h"
#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/reference_counter.h"

template <typename Resource>
class ReferenceCounterWithMetadata : public ReferenceCounter<Resource> {
public:
  ReferenceCounterWithMetadata();

  std::expected<Ref<Resource>, Resource> transferResource(
      Resource&& resource, const MetadataFor<Resource>& metadata);

  // Must be called when related Ref<Resource> is still alive.
  VulkanObjectFor<Resource> getVkResource(HandleFor<Resource> handle) const;

  // Must be called when related Ref<Resource> is still alive.
  const MetadataFor<Resource>& getMetadata(HandleFor<Resource> handle) const;

  size_t size() const;

protected:
  void incrementRefCount(HandleFor<Resource> handle) override;

  void decrementRefCount(HandleFor<Resource> handle) override;

  struct Entry {
    Resource resource;
    MetadataFor<Resource> metadata;
  };

  std::array<Entry, MAX_NUMBER_OF<Resource>> _entries;
  std::array<std::atomic<uint16_t>, MAX_NUMBER_OF<Resource>> _refCounts;
  std::vector<HandleFor<Resource>> _freeHandles;  // TODO: Change to std::inplace_vector.

  mutable std::mutex _mutex;
};

template <typename Resource>
ReferenceCounterWithMetadata<Resource>::ReferenceCounterWithMetadata()
  : _freeHandles(MAX_NUMBER_OF<Resource>) {
  std::iota(_freeHandles.rbegin(), _freeHandles.rend(), HandleFor<Resource>(0));
}

template <typename Resource>
std::expected<Ref<Resource>, Resource> ReferenceCounterWithMetadata<Resource>::transferResource(
    Resource&& resource, const MetadataFor<Resource>& metadata) {
  HandleFor<Resource> handle;
  {
    std::lock_guard lock(_mutex);
    if (_freeHandles.empty()) [[unlikely]] {
      return std::unexpected(std::move(resource));
    }
    handle = _freeHandles.back();
    _freeHandles.pop_back();
  }
  _entries[*handle] = Entry{std::move(resource), metadata};
  return Ref<Resource>(*this, handle);
}

template <typename Resource>
VulkanObjectFor<Resource> ReferenceCounterWithMetadata<Resource>::getVkResource(
    HandleFor<Resource> handle) const {
  return _entries[*handle].resource.getVkResource();
}

template <typename Resource>
const MetadataFor<Resource>& ReferenceCounterWithMetadata<Resource>::getMetadata(
    HandleFor<Resource> handle) const {
  return _entries[*handle].metadata;
}

template <typename Resource>
size_t ReferenceCounterWithMetadata<Resource>::size() const {
  std::lock_guard lock(_mutex);
  return MAX_NUMBER_OF<Resource> - _freeHandles.size();
}

template <typename Resource>
void ReferenceCounterWithMetadata<Resource>::incrementRefCount(HandleFor<Resource> handle) {
  _refCounts[*handle].fetch_add(1, std::memory_order_relaxed);
}

template <typename Resource>
void ReferenceCounterWithMetadata<Resource>::decrementRefCount(HandleFor<Resource> handle) {
  if (_refCounts[*handle].fetch_sub(1, std::memory_order_release) != 1) [[likely]] {
    return;
  }
  std::atomic_thread_fence(std::memory_order_acquire);

  Resource objectToBeDestroyed = std::move(_entries[*handle].resource);
  {
    std::lock_guard lock(_mutex);
    _freeHandles.push_back(handle);
  }
}
