#pragma once

#include "lib/buffer/buffer.h"
#include "primitives.h"

lib::Buffer<VertexP> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions);
lib::Buffer<VertexPT> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords);
lib::Buffer<VertexPTN> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals);
lib::Buffer<VertexPTNT> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals, const lib::Buffer<glm::vec3>& tangents);