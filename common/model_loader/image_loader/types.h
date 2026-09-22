#pragma once

#include <cstdint>
#include <ktx.h>
#include <memory>
#include <stb_image/stb_image.h>
#include <variant>

#include "lib/buffer/buffer.h"

struct StbiDeleter {
  void operator()(stbi_uc* p) const {
    stbi_image_free(p);
  }
};

struct KtxDeleter {
  void operator()(ktxTexture* p) const {
    ktxTexture_Destroy(p);
  }
};

using StbUniquePtr = std::unique_ptr<stbi_uc, StbiDeleter>;
using KtxUniquePtr = std::unique_ptr<ktxTexture, KtxDeleter>;
using OwnedImageData = std::variant<StbUniquePtr, KtxUniquePtr>;

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
  uint32_t width;
  uint32_t height;
  uint32_t mipLevels;
  uint32_t layerCount;
  lib::Buffer<ImageSubresource> subresources;
  const void* data;
  size_t size;
};
