#pragma once

#ifdef __ANDROID__
  #include <android/asset_manager.h>
#endif
#include <string>

#include "common/abstractions/asset_manager.h"
#include "common/file/file_loader.h"
#include "common/model_loader/model_loader.h"

namespace common {

#ifdef __ANDROID__
void setAssetmanager(AAssetManager* assetManager);
#endif

std::vector<AssetData> LoadGltfFromFile(
    common::AssetManager& assetManager, const FileLoader& fileLaoder, const std::string& filePath);

std::vector<AssetData> LoadGltfFromString(
    common::AssetManager& assetManager, const FileLoader& fileLaoder, const std::string& dataString,
    const std::string& baseDir);

}  // namespace common
