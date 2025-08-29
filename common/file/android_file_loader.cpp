#include "android_file_loader.h"

#include <android/asset_manager.h>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"

AndroidFileLoader::AndroidFileLoader(AAssetManager* assetManager) : _assetManager(assetManager) {}

ErrorOr<lib::Buffer<std::byte>> AndroidFileLoader::loadFileToBuffer(
    std::string_view filePath) const {
  AAsset* asset = AAssetManager_open(_assetManager, filePath.data(), AASSET_MODE_BUFFER);
  if (!asset) {
    return Error(EngineError::NOT_FOUND);
  }

  const off_t assetSize = AAsset_getLength(asset);
  lib::Buffer<std::byte> buffer(assetSize);

  int bytesRead = AAsset_read(asset, buffer.data(), assetSize);

  AAsset_close(asset);

  if (bytesRead != assetSize) {
    return Error(EngineError::LOAD_FAILURE);
  }

  return buffer;
}

ErrorOr<std::istringstream> AndroidFileLoader::loadFileToStringStream(
    std::string_view filePath) const {
  ASSIGN_OR_RETURN(const lib::Buffer<std::byte> buffer, loadFileToBuffer(filePath));
  return std::istringstream(std::string(reinterpret_cast<const char*>(buffer.cbegin()),
                                        reinterpret_cast<const char*>(buffer.cend())));
}
