#pragma once

#include "vulkan_lib/model_loader/model_loader.h"
#include "vulkan_lib/status/status.h"

#include <string>
#include <string_view>

ErrorOr<VertexData> loadObj(const std::string& filePath);
