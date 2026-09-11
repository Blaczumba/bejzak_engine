#pragma once

#include <expected>
#include <memory>

#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/reference_counter_with_metadata.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"

class BufferManager final : public ReferenceCounterWithMetadata<Buffer> {
  BufferManager() = default;

public:
  static std::unique_ptr<BufferManager> create();

  ~BufferManager() = default;

  Ref<Buffer> storeBuffer(Buffer&& buffer, const BufferMetadata& metadata);
};
