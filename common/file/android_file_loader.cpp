#include "android_file_loader.h"

#include <android/asset_manager.h>
#include <exception>
#include <format>

#include "lib/buffer/buffer.h"

AndroidFileLoader::AndroidFileLoader(AAssetManager* assetManager) : _assetManager(assetManager) {}

lib::Buffer<std::byte> AndroidFileLoader::loadFileToBuffer(std::string_view filePath) const {
  AAsset* asset = AAssetManager_open(_assetManager, filePath.data(), AASSET_MODE_BUFFER);
  if (!asset) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  const off_t assetSize = AAsset_getLength(asset);
  lib::Buffer<std::byte> buffer(assetSize);

  int bytesRead = AAsset_read(asset, buffer.data(), assetSize);

  AAsset_close(asset);

  if (bytesRead != assetSize) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  return buffer;
}

std::string AndroidFileLoader::loadFileToString(std::string_view filePath) const {
  AAsset* asset = AAssetManager_open(_assetManager, filePath.data(), AASSET_MODE_BUFFER);
  if (!asset) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  const off_t assetSize = AAsset_getLength(asset);
  std::string buffer;
  buffer.resize(assetSize);

  int bytesRead = AAsset_read(asset, buffer.data(), assetSize);

  AAsset_close(asset);

  if (bytesRead != assetSize) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  return buffer;
}
