#include "geometry.h"

bool AABB::contains(const AABB& other) const {
  const glm::vec3 otherLowerCorner = other.lowerCorner;
  const glm::vec3 otherUpperCorner = other.upperCorner;
  return lowerCorner.x <= other.lowerCorner.x && lowerCorner.y <= otherLowerCorner.y
         && lowerCorner.z <= otherLowerCorner.z && upperCorner.x >= otherUpperCorner.x
         && upperCorner.y >= otherUpperCorner.y && upperCorner.z >= otherUpperCorner.z;
}

void AABB::extend(const AABB& other) {
  lowerCorner.x = std::min(lowerCorner.x, other.lowerCorner.x);
  lowerCorner.y = std::min(lowerCorner.y, other.lowerCorner.y);
  lowerCorner.z = std::min(lowerCorner.z, other.lowerCorner.z);

  upperCorner.x = std::max(upperCorner.x, other.upperCorner.x);
  upperCorner.y = std::max(upperCorner.y, other.upperCorner.y);
  upperCorner.z = std::max(upperCorner.z, other.upperCorner.z);
}

AABB createAABBfromVertices(const std::vector<glm::vec3>& vertices, const glm::mat4& transform) {
  AABB volume = {
    .lowerCorner = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()},
    .upperCorner = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(),
                    std::numeric_limits<float>::min()}
  };

  for (glm::vec3 vertex : vertices) {
    vertex = transform * glm::vec4(vertex, 1.0f);
    volume.lowerCorner = {
      std::min(volume.lowerCorner.x, vertex.x), std::min(volume.lowerCorner.y, vertex.y),
      std::min(volume.lowerCorner.z, vertex.z)};

    volume.upperCorner = {
      std::max(volume.upperCorner.x, vertex.x),
      std::max(volume.upperCorner.y, vertex.y),
      std::max(volume.upperCorner.z, vertex.z),
    };
  }

  return volume;
}

std::array<glm::vec4, NUM_CUBE_FACES> extractFrustumPlanes(const glm::mat4& VP) {
  std::array<glm::vec4, NUM_CUBE_FACES> planes = {
    glm::vec4(VP[0][3] + VP[0][0], VP[1][3] + VP[1][0], VP[2][3] + VP[2][0], VP[3][3] + VP[3][0]),
    glm::vec4(VP[0][3] - VP[0][0], VP[1][3] - VP[1][0], VP[2][3] - VP[2][0], VP[3][3] - VP[3][0]),
    glm::vec4(VP[0][3] + VP[0][1], VP[1][3] + VP[1][1], VP[2][3] + VP[2][1], VP[3][3] + VP[3][1]),
    glm::vec4(VP[0][3] - VP[0][1], VP[1][3] - VP[1][1], VP[2][3] - VP[2][1], VP[3][3] - VP[3][1]),
    glm::vec4(VP[0][3] + VP[0][2], VP[1][3] + VP[1][2], VP[2][3] + VP[2][2], VP[3][3] + VP[3][2]),
    glm::vec4(VP[0][3] - VP[0][2], VP[1][3] - VP[1][2], VP[2][3] - VP[2][2], VP[3][3] - VP[3][2])};

  for (size_t i = 0; i < NUM_CUBE_FACES; ++i) {
    glm::normalize(planes[i]);
  }

  return planes;
}

bool AABB::intersectsFrustum(const std::array<glm::vec4, NUM_CUBE_FACES>& planes) const {
  for (const auto& plane : planes) {
    glm::vec3 normal(plane.x, plane.y, plane.z);
    glm::vec3 positiveVertex =
        glm::vec3((plane.x >= 0.0f) ? upperCorner.x : lowerCorner.x,
                  (plane.y >= 0.0f) ? upperCorner.y : lowerCorner.y,
                  (plane.z >= 0.0f) ? upperCorner.z : lowerCorner.z);

    if (glm::dot(normal, positiveVertex) + plane.w < 0.0f) {
      return false;
    }
  }

  return true;
}
