#pragma once

#include <ktx.h>
#include <span>
#include <stb_image/stb_image.h>
#include <string_view>
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan_wrapper/memory_objects/image.h"

struct ImageResource {
  std::variant<stbi_uc*, ktxTexture*> libraryResource;
  ImageDimensions dimensions;
  void* data;
  size_t size;
};

class ImageLoader {
public:
  static ErrorOr<ImageResource> loadCubemapImage(std::string_view imagePath);
  static ErrorOr<ImageResource> load2DImage(std::string_view imagePath);
  static void deallocateResources(ImageResource& resource);
};
