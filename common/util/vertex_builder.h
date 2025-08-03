#pragma once

#include "lib/buffer/buffer.h"
#include "common/status/status.h"
#include "primitives.h"

#include <string_view>
#include <vector>

ErrorOr<lib::Buffer<VertexPT>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords);
ErrorOr<lib::Buffer<VertexPTN>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals);
ErrorOr<lib::Buffer<VertexPTNT>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents);

Status buildInterleavingVertexData(std::span<std::byte> output, std::span<const glm::vec3> positions);
Status buildInterleavingVertexData(std::span<std::byte> output, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords);
Status buildInterleavingVertexData(std::span<std::byte> output, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals);
Status buildInterleavingVertexData(std::span<std::byte> output, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents);