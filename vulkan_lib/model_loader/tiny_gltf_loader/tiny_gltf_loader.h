#pragma once

#include "vulkan_lib/model_loader/model_loader.h"
#include "vulkan_lib/status/status.h"

#include <string>
#include <vector>

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);