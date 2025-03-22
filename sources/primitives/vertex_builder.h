#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "primitives.h"

#include <string_view>
#include <vector>

lib::Buffer<VertexPT> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, size_t size);
lib::Buffer<VertexPTN> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, const glm::vec3* normals, size_t size);
lib::Buffer<VertexPTNT> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, const glm::vec3* normals, const glm::vec3* tangents, size_t size);