#pragma once

#include <string>
#include <vector>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"

ErrorOr<std::vector<VertexData>> LoadGltfFromFile(const std::string& filePath);

ErrorOr<std::vector<VertexData>> LoadGltfFromString(
    const std::string& dataString, const std::string& baseDir);
