#pragma once

#include <string>

#include "common/model_loader/model_loader.h"
#include "common/util/asset_manager.h"

namespace common {

AssetData loadObj(
    common::AssetManager& assetManager, const std::string& name, std::string& stringData);

}  // namespace common
