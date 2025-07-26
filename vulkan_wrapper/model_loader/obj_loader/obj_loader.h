#pragma once

#include "vulkan_wrapper/model_loader/model_loader.h"
#include "vulkan_wrapper/status/status.h"

#include <string>
#include <string_view>

ErrorOr<VertexData> loadObj(const std::string& filePath);
