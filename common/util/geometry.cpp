#include "geometry.h"

#include "common/util/engine_exception.h"

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

template <std::unsigned_integral IndexType>
lib::Buffer<glm::vec3> computeTangents(
    std::span<const IndexType> indices, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords) {
  const size_t vertexCount = positions.size();
  if (texCoords.size() < vertexCount) {
    throw EngineException("Tangent generation requires TEXCOORD_0 for every vertex.");
  }

  // Accumulate per vertex: a vertex shared by N triangles gets their weighted average,
  // not whichever triangle happened to come last.
  std::vector<glm::vec3> accum(vertexCount, glm::vec3(0.0f));

  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    const size_t i0 = indices[i];
    const size_t i1 = indices[i + 1];
    const size_t i2 = indices[i + 2];
    if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
      continue;  // malformed index; skip rather than read out of bounds
    }

    const glm::vec3 edge1 = positions[i1] - positions[i0];
    const glm::vec3 edge2 = positions[i2] - positions[i0];
    const glm::vec2 deltaUV1 = texCoords[i1] - texCoords[i0];
    const glm::vec2 deltaUV2 = texCoords[i2] - texCoords[i0];

    const float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
    if (!(std::abs(det) > 1e-12f)) {
      continue;  // degenerate (or NaN) UVs
    }

    // dP/du. The 1/det factor is not just a scale: its SIGN picks the direction.
    // glTF's v axis points down, so det is negative for ordinary (non-mirrored)
    // triangles; dropping it would flip the tangent on almost the whole mesh.
    const glm::vec3 tangent = (deltaUV2.y * edge1 - deltaUV1.y * edge2) / det;
    const float tangentLen2 = glm::dot(tangent, tangent);
    if (!(tangentLen2 > 0.0f) || !std::isfinite(tangentLen2)) {
      continue;
    }

    // Weight the UNIT tangent by triangle area. Raw dP/du grows as 1/|det|, so without
    // this a sliver with a tiny UV footprint overwhelms every neighbour sharing a vertex.
    const float doubleArea = glm::length(glm::cross(edge1, edge2));
    const glm::vec3 weighted = tangent * (doubleArea / std::sqrt(tangentLen2));

    accum[i0] += weighted;
    accum[i1] += weighted;
    accum[i2] += weighted;
  }

  lib::Buffer<glm::vec3> tangents(vertexCount);
  for (size_t v = 0; v < vertexCount; ++v) {
    const float len2 = glm::dot(accum[v], accum[v]);
    // Fallback for vertices touching only degenerate triangles. The vertex shader
    // re-orthogonalises against the normal (and handles the parallel case).
    tangents[v] = len2 > std::numeric_limits<float>::min() ? accum[v] / std::sqrt(len2) :
                                                             glm::vec3(1.0f, 0.0f, 0.0f);
  }
  return tangents;
}

template <std::unsigned_integral IndexType>
lib::Buffer<glm::vec3> computeTangentsFromBytes(
    std::span<const std::byte> indicesBytes, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords) {
  const std::span<const IndexType> indices(reinterpret_cast<const IndexType*>(indicesBytes.data()),
                                           indicesBytes.size() / sizeof(IndexType));
  return computeTangents(indices, positions, texCoords);
}

}  // namespace

lib::Buffer<glm::vec3> createTangents(
    uint8_t indexSize, std::span<const std::byte> indicesBytes,
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
  // Dispatch before dividing by indexSize (the old code divided first, so 0 was UB).
  switch (indexSize) {
    case 1:
      return computeTangentsFromBytes<uint8_t>(indicesBytes, positions, texCoords);
    case 2:
      return computeTangentsFromBytes<uint16_t>(indicesBytes, positions, texCoords);
    case 4:
      return computeTangentsFromBytes<uint32_t>(indicesBytes, positions, texCoords);
  }
  throw EngineException("Not recognized index type.");
}
