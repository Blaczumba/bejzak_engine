#include "vertex_builder.h"

#include "lib/status/status.h"

lib::Buffer<VertexPT> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, size_t size) {
	lib::Buffer<VertexPT> vertices(size);
	std::transform(positions, positions + size, texCoords, vertices.begin(),
		[](const glm::vec3& pos, const glm::vec2& texCoord) { return VertexPT{ pos, texCoord }; });
	return vertices;
}

lib::Buffer<VertexPTN> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, const glm::vec3* normals, size_t size) {
	lib::Buffer<VertexPTN> vertices(size);
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
	}
	return vertices;
}

lib::Buffer<VertexPTNT> buildInterleavingVertexData(const glm::vec3* positions, const glm::vec2* texCoords, const glm::vec3* normals, const glm::vec3* tangents, size_t size) {
	lib::Buffer<VertexPTNT> vertices(size);
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
		vertices[i].tangent = tangents[i];
	}
	return vertices;
}

template <BufferLike... Buffer>
void buildInterleavingVertexDataAsBytes(void* destinationBuffer, const Buffer&... buffers, size_t size) {
	std::byte* dst = static_cast<std::byte*>(destinationBuffer);
	for (size_t i = 0; i < size; ++i) {
		((std::memcpy(dst, &buffers[i], sizeof(buffers[i])), dst += sizeof(buffers[i])), ...);
	}
}