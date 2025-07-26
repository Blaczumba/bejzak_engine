#pragma once

#include "vulkan_wrapper/model_loader/model_loader.h"
#include "vulkan_wrapper/status/status.h"

#include <string>
#include <vector>

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);