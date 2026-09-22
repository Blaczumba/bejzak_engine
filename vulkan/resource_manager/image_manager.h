#pragma once

#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/resource_manager_allocation_strategy.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/image.h"

class ImageManager final {
public:
  ImageManager() = default;

  ~ImageManager() = default;

  Ref<Image> storeImage(Image&& image, const ImageMetadata& metadata);

private:
  AllocationStrategy<Image, AllocationPolicy::POOL_BASED> _allocationStrategy;
};
