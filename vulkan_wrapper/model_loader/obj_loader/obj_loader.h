#pragma once

#include <string>
#include <string_view>

#include "common/status/status.h"
#include "vulkan_wrapper/model_loader/model_loader.h"

ErrorOr<VertexData> loadObj(const std::string& filePath);
