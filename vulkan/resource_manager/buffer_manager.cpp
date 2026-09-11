#include "vulkan/resource_manager/buffer_manager.h"

#include <memory>

#include "vulkan/resource_manager/ref.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"

std::unique_ptr<BufferManager> BufferManager::create() {
  return std::unique_ptr<BufferManager>(new BufferManager());
}

Ref<Buffer> BufferManager::storeBuffer(Buffer&& buffer, const BufferMetadata& metadata) {
  return *transferResource(std::move(buffer), metadata);
}
