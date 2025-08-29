#pragma once

#include "file_loader.h"

#include <android/asset_manager.h>
#include <sstream>

#include "lib/buffer/buffer.h"
#include "common/status/status.h"

class AndroidFileLoader : public FileLoader {
 public:
  AndroidFileLoader(AAssetManager* assetManager);

  ~AndroidFileLoader() override = default;

  ErrorOr<lib::Buffer<std::byte>> loadFileToBuffer(std::string_view filePath) const override;

  ErrorOr<std::istringstream> loadFileToStringStream(std::string_view filePath) const override;

 private:
  AAssetManager* _assetManager;
};