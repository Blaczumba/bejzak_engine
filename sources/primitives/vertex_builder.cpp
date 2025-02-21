#include "vertex_builder.h"

lib::Buffer<VertexP> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions) {
	// TODO: to be implemented
	return lib::Buffer<VertexP>(positions.size());
}

lib::Buffer<VertexPT> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords) {
	lib::Buffer<VertexPT> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), texCoords.cbegin(), vertices.begin(),
		[](const glm::vec3& pos, const glm::vec2& texCoord) { return VertexPT{ pos, texCoord }; });
	return vertices;
}

lib::Buffer<VertexPTN> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals) {
	lib::Buffer<VertexPTN> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i] = VertexPTN{ positions[i], texCoords[i], normals[i] };
	}
	return vertices;
}

lib::Buffer<VertexPTNT> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals, const lib::Buffer<glm::vec3>& tangents) {
	lib::Buffer<VertexPTNT> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i] = VertexPTNT{ positions[i], texCoords[i], normals[i], tangents[i] };
	}
	return vertices;
}