#pragma once

#include <expected>
#include <memory>

#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/reference_counter_with_metadata.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/image.h"

class ImageManager final : public ReferenceCounterWithMetadata<Image> {
  ImageManager() = default;

public:
  static std::unique_ptr<ImageManager> create();

  ~ImageManager() = default;

  Ref<Image> storeImage(Image&& image, const ImageMetadata& metadata);
};
