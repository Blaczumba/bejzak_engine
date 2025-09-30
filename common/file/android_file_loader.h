#pragma once

#include <android/asset_manager.h>
#include <string>

#include "common/status/status.h"
#include "file_loader.h"
#include "lib/buffer/buffer.h"

class AndroidFileLoader : public FileLoader {
public:
  AndroidFileLoader(AAssetManager* assetManager);

  ~AndroidFileLoader() override = default;

  ErrorOr<lib::Buffer<std::byte>> loadFileToBuffer(std::string_view filePath) const override;

  ErrorOr<std::string> loadFileToStringStream(std::string_view filePath) const override;

private:
  AAssetManager* _assetManager;
};
