#pragma once

#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <limits>
#include <span>
#include <vector>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"

constexpr size_t NUM_CUBE_FACES = 6;

struct AABB {
  glm::vec3 lowerCorner;
  glm::vec3 upperCorner;

  bool contains(const AABB& other) const;
  bool intersectsFrustum(std::span<const glm::vec4> planes) const;
  void extend(const AABB& other);
};

AABB createAABBfromVertices(
    std::span<const glm::vec3> vertices, const glm::mat4& transform = glm::mat4(1.0f));

std::array<glm::vec4, NUM_CUBE_FACES> extractFrustumPlanes(const glm::mat4& VP);

ErrorOr<lib::Buffer<glm::vec3>> createTangents(
    uint8_t indexSize, std::span<const std::byte> indicesBytes,
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords);
