#include "vulkan/resource_manager/image_manager.h"

#include "vulkan/resource_manager/ref.h"
#include "vulkan/wrapper/memory_objects/image.h"

Ref<Image> ImageManager::storeImage(Image&& image, const ImageMetadata& metadata) {
  return _allocationStrategy.transferResource(std::move(image), metadata);
}
