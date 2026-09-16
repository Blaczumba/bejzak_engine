#pragma once

#include <memory>
#include <optional>
#include <utility>

#include "vulkan/resource_manager/reference_counter_with_metadata.h"

enum class AllocationPolicy : uint8_t {
  POOL_BASED
};

template <typename Resource, AllocationPolicy policy>
class AllocationStrategy {};

template <typename Resource>
class AllocationStrategy<Resource, AllocationPolicy::POOL_BASED> {
public:
  AllocationStrategy() {
    _counters.push_back(std::make_unique<ReferenceCounterWithMetadata<Resource>>());
  }

  Ref<Resource> transferResource(Resource&& resource, const MetadataFor<Resource>& metadata) {
    std::expected<Ref<VirtualAllocation>, ReferenceCounterWithMetadata<VirtualAllocation>::Error>
        expectedRef;
    for (
        std::unique_ptr<ReferenceCounterWithMetadata<VirtualAllocation>>& virtualAllocationCounter :
        _counters) {
      // Fast path: virtual allocation counters have a free spot.
      expectedRef = virtualAllocationCounter->transferResource(std::move(resource), metadata);
      if (expectedRef.has_value()) {
        break;
      }
    }

    if (!expectedRef.has_value()) [[unlikely]] {
      // Slow path: very rare, if no virtual allocation counter has free spot then allocate the
      // new one or take the spare one.
      _counters.push_back(_counterToBeReclaimed.has_value() ?
                              std::move(*_counterToBeReclaimed) :
                              std::make_unique<ReferenceCounterWithMetadata<VirtualAllocation>>());
      _counterToBeReclaimed.reset();
      expectedRef = _counters.back()->transferResource(std::move(resource), metadata);
      if (!expectedRef.has_value()) [[unlikely]] {
        throw EngineException("Failed to store the virtual allocation.");
      }
    }
    return std::move(*expectedRef);
  }

  void cleanEmptyCounters() {
    while (_counters.size() > 1 && _counters.back()->size() == 0) {
      if (!_counterToBeReclaimed.has_value()) {
        _counterToBeReclaimed = std::move(_counters.back());
      }
      _counters.pop_back();
    }
  }

private:
  std::vector<std::unique_ptr<ReferenceCounterWithMetadata<Resource>>> _counters;
  // TODO: Change to std::inplace_vector
  std::optional<std::unique_ptr<ReferenceCounterWithMetadata<Resource>>> _counterToBeReclaimed;
};
