#pragma once

#include "model_loader/model_loader.h"
#include "status/status.h"

#include <string>
#include <string_view>

ErrorOr<VertexData> loadObj(const std::string& filePath);
