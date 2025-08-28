#pragma once

#include "file_loader.h"

#include <android/asset_manager.h>

#include "lib/buffer/buffer.h"
#include "common/status/status.h"

class AndroidFileLoader : public FileLoader {
 public:
  AndroidFileLoader(AAssetManager* assetManager);

  ErrorOr<lib::Buffer<std::byte>> loadFile(std::string_view filePath) const override;

 private:
  AAssetManager* _assetManager;
};