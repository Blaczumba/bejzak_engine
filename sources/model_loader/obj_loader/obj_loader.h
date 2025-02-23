#pragma once

#include "model_loader/model_loader.h"

#include <expected>
#include <string>
#include <string_view>

std::expected<VertexData, std::string_view> loadObj(const std::string& filePath);
