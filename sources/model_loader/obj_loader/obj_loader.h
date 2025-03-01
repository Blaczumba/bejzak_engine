#pragma once

#include "lib/status/status.h"
#include "model_loader/model_loader.h"

#include <string>
#include <string_view>

lib::ErrorOr<VertexData> loadObj(const std::string& filePath);
