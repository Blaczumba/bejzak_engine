#pragma once

#include "model_loader/model_loader.h"
#include "status/status.h"

#include <string>
#include <vector>

ErrorOr<std::vector<VertexData>> LoadGltf(const std::string& filePath);