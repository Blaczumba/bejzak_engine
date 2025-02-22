#pragma once

#include "lib/buffer/buffer.h"
#include "primitives.h"

#include <expected>
#include <string_view>
#include <vector>

std::expected<lib::Buffer<VertexP>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions);
std::expected<lib::Buffer<VertexPT>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords);
std::expected<lib::Buffer<VertexPTN>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals);
std::expected<lib::Buffer<VertexPTNT>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents);