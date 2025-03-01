#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "primitives.h"

#include <string_view>
#include <vector>

lib::ErrorOr<lib::Buffer<VertexP>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions);
lib::ErrorOr<lib::Buffer<VertexPT>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords);
lib::ErrorOr<lib::Buffer<VertexPTN>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals);
lib::ErrorOr<lib::Buffer<VertexPTNT>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents);