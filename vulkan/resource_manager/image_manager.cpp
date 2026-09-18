#include "vulkan/resource_manager/image_manager.h"

#include <memory>

#include "vulkan/resource_manager/ref.h"
#include "vulkan/wrapper/memory_objects/image.h"

std::unique_ptr<ImageManager> ImageManager::create() {
  return std::unique_ptr<ImageManager>(new ImageManager());
}

Ref<Image> ImageManager::storeImage(Image&& image, const ImageMetadata& metadata) {
  return _allocationStrategy.transferResource(std::move(image), metadata);
}
