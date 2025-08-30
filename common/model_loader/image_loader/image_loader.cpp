#include "image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <cmath>
#include <span>
#include <stb_image/stb_image.h>
#include <vector>

ErrorOr<ImageResource> ImageLoader::loadImageStbi(std::span<const std::byte> imageData) {
  int width, height, channels;
  stbi_uc* pixels = stbi_load_from_memory(
      reinterpret_cast<const stbi_uc*>(imageData.data()), static_cast<int>(imageData.size()),
      &width, &height, &channels, STBI_rgb_alpha);
  if (!pixels) {
    return Error(EngineError::LOAD_FAILURE);
  }

  return ImageResource{
    .libraryResource = pixels,
    .width = static_cast<uint32_t>(width),
    .height = static_cast<uint32_t>(height),
    .mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1,
    .layerCount = 1,
    .subresources = {ImageSubresource{
        .layerCount = 1,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .depth = 1,
    }},
    .data = pixels,
    .size = static_cast<uint32_t>(4 * width * height)};
}

ErrorOr<ImageResource> ImageLoader::loadImageKtx(std::span<const std::byte> imageData) {
  ktxTexture* ktxTexture;
  if (ktxResult result = ktxTexture_CreateFromMemory(
          reinterpret_cast<const ktx_uint8_t*>(imageData.data()), imageData.size(),
          KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
      result != KTX_SUCCESS) {
    return Error(EngineError::LOAD_FAILURE);
  }

  ImageResource image{
    .libraryResource = ktxTexture,
    .width = ktxTexture->baseWidth,
    .height = ktxTexture->baseHeight,
    .mipLevels = ktxTexture->numLevels,
    .layerCount = 6,
    .data = ktxTexture->pData,
    .size = ktxTexture->dataSize
  };

  for (uint32_t face = 0; face < image.layerCount; ++face) {
    for (uint32_t level = 0; level < image.mipLevels; ++level) {
      // Calculate offset into staging buffer for the current mip level and face
      ktx_size_t offset;
      if (ktxResult result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
          result != KTX_SUCCESS) {
        ktxTexture_Destroy(ktxTexture);
        return Error(EngineError::LOAD_FAILURE);
      }

      image.subresources
          .push_back(ImageSubresource{
              .offset = offset,
              .mipLevel = level,
              .baseArrayLayer = face,
              .layerCount = 1,
              .width = image.width >> level,
              .height = image.height >> level,
              .depth = 1,
      });
    }
  }
  return image;
}

namespace {

struct Deallocator {
  void operator()(ktxTexture* texture) {
    ktxTexture_Destroy(texture);
  }

  void operator()(stbi_uc* texture) {
    stbi_image_free(texture);
  }

  void operator()(auto&&) {}
};

}  // namespace

void ImageLoader::deallocateResources(ImageResource& resource) {
  std::visit(Deallocator{}, resource.libraryResource);
}
