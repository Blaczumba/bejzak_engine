#pragma once

#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

constexpr size_t NUM_CUBE_FACES = 6;

struct AABB {
  glm::vec3 lowerCorner;
  glm::vec3 upperCorner;

  bool contains(const AABB& other) const;
  bool intersectsFrustum(const std::array<glm::vec4, NUM_CUBE_FACES>& planes) const;
  void extend(const AABB& other);
};

AABB createAABBfromVertices(
    const std::vector<glm::vec3>& vertices, const glm::mat4& transform = glm::mat4(1.0f));
std::array<glm::vec4, NUM_CUBE_FACES> extractFrustumPlanes(const glm::mat4& VP);
