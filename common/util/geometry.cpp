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

AABB createAABBfromVertices(std::span<const glm::vec3> vertices, const glm::mat4& transform) {
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
  return std::array<glm::vec4, NUM_CUBE_FACES>{
    glm::normalize(glm::vec4(
        VP[0][3] + VP[0][0], VP[1][3] + VP[1][0], VP[2][3] + VP[2][0], VP[3][3] + VP[3][0])),
    glm::normalize(glm::vec4(
        VP[0][3] - VP[0][0], VP[1][3] - VP[1][0], VP[2][3] - VP[2][0], VP[3][3] - VP[3][0])),
    glm::normalize(glm::vec4(
        VP[0][3] + VP[0][1], VP[1][3] + VP[1][1], VP[2][3] + VP[2][1], VP[3][3] + VP[3][1])),
    glm::normalize(glm::vec4(
        VP[0][3] - VP[0][1], VP[1][3] - VP[1][1], VP[2][3] - VP[2][1], VP[3][3] - VP[3][1])),
    glm::normalize(glm::vec4(
        VP[0][3] + VP[0][2], VP[1][3] + VP[1][2], VP[2][3] + VP[2][2], VP[3][3] + VP[3][2])),
    glm::normalize(glm::vec4(
        VP[0][3] - VP[0][2], VP[1][3] - VP[1][2], VP[2][3] - VP[2][2], VP[3][3] - VP[3][2]))};
}

bool AABB::intersectsFrustum(std::span<const glm::vec4> planes) const {
  for (const glm::vec4& plane : planes) {
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

namespace {

template <typename IndexType>
std::enable_if_t<std::is_unsigned<IndexType>::value, lib::Buffer<glm::vec3>> processTangents(
    std::span<const IndexType> indices, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords) {
  lib::Buffer<glm::vec3> tangents(positions.size());
  for (size_t i = 0; i < indices.size(); i += 3) {
    const glm::vec3& pos0 = positions[indices[i]];
    const glm::vec3 edge1 = positions[indices[i + 1]] - pos0;
    const glm::vec3 edge2 = positions[indices[i + 2]] - pos0;

    const glm::vec2& texCoord0 = texCoords[indices[i]];
    const glm::vec2 deltaUV1 = texCoords[indices[i + 1]] - texCoord0;
    const glm::vec2 deltaUV2 = texCoords[indices[i + 2]] - texCoord0;

    // float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    tangents[indices[i]] = tangents[indices[i + 1]] = tangents[indices[i + 2]] =
        glm::normalize(deltaUV2.y * edge1 - deltaUV1.y * edge2);  // f scale
    // glm::vec3 bitangent = glm::normalize(-deltaUV2.x * edge1 + deltaUV1.x * edge2); // f scale
  }
  return tangents;
}

}  // namespace

ErrorOr<lib::Buffer<glm::vec3>> createTangents(
    uint8_t indexSize, std::span<const std::byte> indicesBytes,
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
  const size_t indicesCount = indicesBytes.size() / indexSize;
  switch (indexSize) {
    case 1:
      return processTangents(
          std::span(reinterpret_cast<const uint8_t*>(indicesBytes.data()), indicesCount), positions,
          texCoords);
    case 2:
      return processTangents(
          std::span(reinterpret_cast<const uint16_t*>(indicesBytes.data()), indicesCount),
          positions, texCoords);
    case 4:
      return processTangents(
          std::span(reinterpret_cast<const uint32_t*>(indicesBytes.data()), indicesCount),
          positions, texCoords);
  }
  return Error(EngineError::NOT_RECOGNIZED_TYPE);
}
