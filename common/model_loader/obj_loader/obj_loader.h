#pragma once

#include <string>

#include "common/abstractions/asset_manager.h"
#include "common/model_loader/model_loader.h"

namespace common {

AssetData loadObj(
    common::AssetManager& assetManager, const std::string& name, std::string& stringData);

}  // namespace common
