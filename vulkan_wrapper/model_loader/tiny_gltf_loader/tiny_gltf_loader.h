#pragma once

#include "vulkan_wrapper/model_loader/model_loader.h"
#include "common/status/status.h"

#include <string>
#include <vector>

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);