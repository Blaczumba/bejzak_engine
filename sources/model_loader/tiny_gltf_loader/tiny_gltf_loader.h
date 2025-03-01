#pragma once

#include "model_loader/model_loader.h"

#include <expected>
#include <string>
#include <vector>

std::expected<std::vector<VertexData>, std::string_view> LoadGltf(const std::string& filePath);