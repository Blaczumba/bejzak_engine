#include "vertex_builder.h"

#include "status/status.h"

// Function to build interleaved vertex data for position and texture coordinates
ErrorOr<lib::Buffer<VertexPT>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
    if (positions.size() != texCoords.size()) [[unlikely]] {
        return Error(EngineError::SIZE_MISMATCH);
    }

    lib::Buffer<VertexPT> vertices(positions.size());
    std::transform(positions.cbegin(), positions.cend(), texCoords.cbegin(), vertices.begin(),
        [](const glm::vec3& pos, const glm::vec2& texCoord) {
        return VertexPT{ pos, texCoord };
    });

    return vertices;
}

// Function to build interleaved vertex data for position, texture coordinates, and normals
ErrorOr<lib::Buffer<VertexPTN>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals) {
    if (positions.size() != texCoords.size() || positions.size() != normals.size()) [[unlikely]] {
        throw Error(EngineError::SIZE_MISMATCH);
    }

    lib::Buffer<VertexPTN> vertices(positions.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].pos = positions[i];
        vertices[i].texCoord = texCoords[i];
        vertices[i].normal = normals[i];
    }
    return vertices;
}

// Function to build interleaved vertex data for position, texture coordinates, normals, and tangents
ErrorOr<lib::Buffer<VertexPTNT>> buildInterleavingVertexData(std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
    if (positions.size() != texCoords.size() || positions.size() != normals.size() || positions.size() != tangents.size()) [[unlikely]] {
        throw Error(EngineError::SIZE_MISMATCH);
    }

    lib::Buffer<VertexPTNT> vertices(positions.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].pos = positions[i];
        vertices[i].texCoord = texCoords[i];
        vertices[i].normal = normals[i];
        vertices[i].tangent = tangents[i];
    }
    return vertices;
}
