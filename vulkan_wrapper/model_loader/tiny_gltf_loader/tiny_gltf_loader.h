#pragma once

#include <string>
#include <vector>

#include "common/status/status.h"
#include "vulkan_wrapper/model_loader/model_loader.h"

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);
