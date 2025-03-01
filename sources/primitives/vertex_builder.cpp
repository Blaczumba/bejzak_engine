#include "vertex_builder.h"

#include "lib/status/status.h"

constexpr std::string_view NOT_MATCHING_SIZES = "Sizes of input data do not match.";
constexpr std::string_view EMPTY_INPUT_DATA = "Input data is empty.";

lib::ErrorOr<lib::Buffer<VertexP>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions) {
	if (positions.size() == 0)
		return lib::Error(EMPTY_INPUT_DATA);
	lib::Buffer<VertexP> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), vertices.begin(),
		[](const glm::vec3& pos) { return VertexP{ pos }; });
	return vertices;
}

lib::ErrorOr<lib::Buffer<VertexPT>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords) {
	if (positions.size() != texCoords.size())
		return lib::Error(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return lib::Error(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPT> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), texCoords.cbegin(), vertices.begin(),
		[](const glm::vec3& pos, const glm::vec2& texCoord) { return VertexPT{ pos, texCoord }; });
	return vertices;
}

lib::ErrorOr<lib::Buffer<VertexPTN>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size())
		return lib::Error(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return lib::Error(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTN> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
	}
	return vertices;
}

lib::ErrorOr<lib::Buffer<VertexPTNT>> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size() || positions.size() != tangents.size())
		return lib::Error(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return lib::Error(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTNT> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
		vertices[i].tangent = tangents[i];
	}
	return vertices;
}