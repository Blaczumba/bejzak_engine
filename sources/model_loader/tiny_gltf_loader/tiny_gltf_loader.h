#pragma once

#include "model_loader/model_loader.h"

#include <string>
#include <vector>

std::vector<VertexData> LoadGltf(const std::string& filePath);