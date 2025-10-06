#include "vertex_builder.h"

#include "common/status/status.h"

ErrorOr<lib::Buffer<VertexPT>> buildInterleavingVertexData(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
  if (positions.size() != texCoords.size()) [[unlikely]] {
    return Error(EngineError::SIZE_MISMATCH);
  }

  lib::Buffer<VertexPT> vertices(positions.size());
  std::transform(std::cbegin(positions), std::cend(positions), std::cbegin(texCoords),
                 vertices.begin(), [](const glm::vec3& pos, const glm::vec2& texCoord) {
                   return VertexPT{pos, texCoord};
                 });

  return vertices;
}

ErrorOr<lib::Buffer<VertexPTN>> buildInterleavingVertexData(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals) {
  if (positions.size() != texCoords.size() || positions.size() != normals.size()) [[unlikely]] {
    throw Error(EngineError::SIZE_MISMATCH);
  }

  lib::Buffer<VertexPTN> vertices(positions.size());
  for (size_t i = 0; i < vertices.size(); ++i) {
    vertices[i] = VertexPTN{positions[i], texCoords[i], normals[i]};
  }
  return vertices;
}

ErrorOr<lib::Buffer<VertexPTNT>> buildInterleavingVertexData(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
  if (positions.size() != texCoords.size() || positions.size() != normals.size()
      || positions.size() != tangents.size()) [[unlikely]] {
    throw Error(EngineError::SIZE_MISMATCH);
  }

  lib::Buffer<VertexPTNT> vertices(positions.size());
  for (size_t i = 0; i < vertices.size(); ++i) {
    vertices[i] = VertexPTNT{positions[i], texCoords[i], normals[i], tangents[i]};
  }
  return vertices;
}

Status buildInterleavingVertexData(
    std::span<std::byte> output, std::span<const glm::vec3> positions) {
  if (positions.size() != output.size() / sizeof(glm::vec3)) [[unlikely]] {
    return Error(EngineError::SIZE_MISMATCH);
  }

  std::memcpy(output.data(), positions.data(), output.size());
  return StatusOk();
}

Status buildInterleavingVertexData(
    std::span<std::byte> output, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords) {
  static constexpr size_t vertexSize = sizeof(glm::vec3) + sizeof(glm::vec2);
  if (output.size() != positions.size() * vertexSize || positions.size() != texCoords.size())
      [[unlikely]] {
    return Error(EngineError::SIZE_MISMATCH);
  }

  auto* vertexPtr = reinterpret_cast<VertexPT*>(output.data());

  for (size_t i = 0; i < positions.size(); ++i) {
    vertexPtr[i] = VertexPT{
      positions[i],
      texCoords[i],
    };
  }
  return StatusOk();
}

Status buildInterleavingVertexData(
    std::span<std::byte> output, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals) {
  static constexpr size_t vertexSize = sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3);
  if (output.size() != positions.size() * vertexSize || positions.size() != texCoords.size()
      || positions.size() != normals.size()) [[unlikely]] {
    return Error(EngineError::SIZE_MISMATCH);
  }

  auto* vertexPtr = reinterpret_cast<VertexPTN*>(output.data());

  for (size_t i = 0; i < positions.size(); ++i) {
    vertexPtr[i] = VertexPTN{
      positions[i],
      texCoords[i],
      normals[i],
    };
  }
  return StatusOk();
}

Status buildInterleavingVertexData(
    std::span<std::byte> output, std::span<const glm::vec3> positions,
    std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals,
    std::span<const glm::vec3> tangents) {
  static constexpr size_t vertexSize =
      sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3) + sizeof(glm::vec3);
  if (output.size() != positions.size() * vertexSize || positions.size() != texCoords.size()
      || positions.size() != normals.size() || positions.size() != tangents.size()) [[unlikely]] {
    return Error(EngineError::SIZE_MISMATCH);
  }

  auto* vertexPtr = reinterpret_cast<VertexPTNT*>(output.data());

  for (size_t i = 0; i < positions.size(); ++i) {
    vertexPtr[i] = VertexPTNT{positions[i], texCoords[i], normals[i], tangents[i]};
  }
  return StatusOk();
}
