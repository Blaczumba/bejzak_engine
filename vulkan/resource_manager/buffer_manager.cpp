#include "vulkan/resource_manager/buffer_manager.h"

#include "vulkan/resource_manager/ref.h"
#include "vulkan/wrapper/memory_objects/buffer.h"

Ref<Buffer> BufferManager::storeBuffer(Buffer&& buffer, const BufferMetadata& metadata) {
  return _allocationStrategy.transferResource(std::move(buffer), metadata);
}
