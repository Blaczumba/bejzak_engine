#pragma once

#include <atomic>
#include <condition_variable>
#include <format>
#include <mutex>
#include <numeric>
#include <utility>

#include "common/util/engine_exception.h"
#include "lib/sparse/sparse_map.h"
#include "vulkan/resource_manager/handle.h"
#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/reference_counter.h"

template <typename Resource>
class ReferenceCounterWithHashing : public ReferenceCounter<Resource> {
public:
  ReferenceCounterWithHashing();

  Ref<Resource> getOrCreateResource(
      std::
          function<Resource(const LogicalDevice&, const MetadataFor<Resource>&)>&& creationFunction,
      const LogicalDevice& logicalDevice, const MetadataFor<Resource>& metadata);

  // Should be moved to protected session.
  // Must be called when related Ref<Resource> is still alive.
  VulkanObjectFor<Resource> getVkResource(HandleFor<Resource> handle) const override;

  // Must be called when related Ref<Resource> is still alive.
  const MetadataFor<Resource>& getMetadata(HandleFor<Resource> handle) const override;

  // Must be called when related Ref<Resource> is still alive.
  std::tuple<VulkanObjectFor<Resource>, const MetadataFor<Resource>&> getVkResourceWithMetadata(
      HandleFor<Resource> handle) const override;

protected:
  void incrementRefCount(HandleFor<Resource> handle) override;

  void decrementRefCount(HandleFor<Resource> handle) override;

  struct Entry {
    Resource resource;
    const MetadataFor<Resource>* metadata;
  };

  struct CacheSlot {
    HandleFor<Resource> handle;
    bool ready = false;
  };

  std::array<Entry, MAX_NUMBER_OF<Resource>> _entries;
  std::array<std::atomic<uint16_t>, MAX_NUMBER_OF<Resource>> _refCounts{};
  std::vector<HandleFor<Resource>> _freeHandles;  // TODO: Change to std::inplace_vector.

  std::unordered_map<MetadataFor<Resource>, CacheSlot, HasherFor<Resource>> _collisionMap;

  std::condition_variable _creationDone;
  mutable std::mutex _mutex;
};

template <typename Resource>
ReferenceCounterWithHashing<Resource>::ReferenceCounterWithHashing()
  : _freeHandles(MAX_NUMBER_OF<Resource>) {
  std::iota(_freeHandles.rbegin(), _freeHandles.rend(), HandleFor<Resource>(0));
}

template <typename Resource>
Ref<Resource> ReferenceCounterWithHashing<Resource>::getOrCreateResource(
    std::function<Resource(const LogicalDevice&, const MetadataFor<Resource>&)>&& creationFunction,
    const LogicalDevice& logicalDevice, const MetadataFor<Resource>& metadata) {
  HandleFor<Resource> handle;
  const MetadataFor<Resource>* metadataPtr;
  bool* readyPtr;

  {
    std::unique_lock lock(_mutex);

    auto [it, inserted] = _collisionMap.try_emplace(metadata);
    while (!inserted && !it->second.ready) {
      _creationDone.wait(lock);
      std::tie(it, inserted) = _collisionMap.try_emplace(metadata);
    }

    if (!inserted) [[likely]] {
      return Ref<Resource>(*this, it->second.handle);
    }

    if (_freeHandles.empty()) [[unlikely]] {
      _collisionMap.erase(it);
      throw EngineException(
          std::format("No free handles available for {} creation.", NAME_OF<Resource>));
    }

    handle = _freeHandles.back();
    _freeHandles.pop_back();
    it->second.handle = handle;
    metadataPtr = &it->first;
    readyPtr = &it->second.ready;
  }

  Resource created = creationFunction(logicalDevice, metadata);

  Ref<Resource> ref;
  {
    std::lock_guard lock(_mutex);
    _entries[*handle] = Entry{std::move(created), metadataPtr};
    *readyPtr = true;
    ref = Ref<Resource>(*this, handle);
  }

  _creationDone.notify_all();
  return ref;
}

template <typename Resource>
VulkanObjectFor<Resource> ReferenceCounterWithHashing<Resource>::getVkResource(
    HandleFor<Resource> handle) const {
  return _entries[*handle].resource.getVkResource();
}

template <typename Resource>
const MetadataFor<Resource>& ReferenceCounterWithHashing<Resource>::getMetadata(
    HandleFor<Resource> handle) const {
  return *_entries[*handle].metadata;
}

template <typename Resource>
std::tuple<VulkanObjectFor<Resource>, const MetadataFor<Resource>&>
ReferenceCounterWithHashing<Resource>::getVkResourceWithMetadata(HandleFor<Resource> handle) const {
  return std::make_tuple(
      _entries[*handle].resource.getVkResource(), std::cref(*_entries[*handle].metadata));
}

template <typename Resource>
void ReferenceCounterWithHashing<Resource>::incrementRefCount(HandleFor<Resource> handle) {
  _refCounts[*handle].fetch_add(1, std::memory_order_relaxed);
}

template <typename Resource>
void ReferenceCounterWithHashing<Resource>::decrementRefCount(HandleFor<Resource> handle) {
  if (_refCounts[*handle].fetch_sub(1, std::memory_order_release) != 1) [[likely]] {
    return;
  }
  std::atomic_thread_fence(std::memory_order_acquire);

  Resource objectToBeDestroyed;  // Destroyed after the lock is released.
  {
    std::lock_guard lock(_mutex);

    // Resurrection guard.
    if (_refCounts[*handle].load(std::memory_order_relaxed) != 0) [[unlikely]] {
      return;
    }

    Entry& entry = _entries[*handle];
    objectToBeDestroyed = std::move(entry.resource);
    _collisionMap.erase(*entry.metadata);
    _freeHandles.push_back(handle);
  }
}
