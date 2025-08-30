#pragma once

#include <ktx.h>
#include <span>
#include <stb_image/stb_image.h>
#include <string_view>
#include <variant>
#include <vector>

#include "common/status/status.h"

struct ImageSubresource {
  size_t offset;
  uint32_t mipLevel;
  uint32_t baseArrayLayer;
  uint32_t layerCount;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
};

struct ImageResource {
  std::variant<stbi_uc*, ktxTexture*> libraryResource;
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels;
  uint32_t layerCount;
  std::vector<ImageSubresource> subresources;
  void* data;
  size_t size;
};

class ImageLoader {
public:
  static ErrorOr<ImageResource> loadImageStbi(std::span<const std::byte> imagePath);
  static ErrorOr<ImageResource> loadImageKtx(std::span<const std::byte> imagePath);
  static void deallocateResources(ImageResource& resource);
};
