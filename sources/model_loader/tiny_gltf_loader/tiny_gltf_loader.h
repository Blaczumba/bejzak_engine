#pragma once

#include "lib/status/status.h"
#include "model_loader/model_loader.h"

#include <expected>
#include <string>
#include <vector>

lib::ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);