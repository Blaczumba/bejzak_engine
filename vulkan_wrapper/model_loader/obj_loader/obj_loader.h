#pragma once

#include <sstream>

#include "common/status/status.h"
#include "vulkan_wrapper/model_loader/model_loader.h"

ErrorOr<VertexData> loadObj(std::istringstream& data);
