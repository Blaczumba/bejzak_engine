#include "vertex_builder.h"

constexpr std::string_view NOT_MATCHING_SIZES = "Sizes of input data do not match.";
constexpr std::string_view EMPTY_INPUT_DATA = "Input data is empty.";

std::expected<lib::Buffer<VertexP>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions) {
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexP> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), vertices.begin(),
		[](const glm::vec3& pos) { return VertexP{ pos }; });
	return vertices;
}

std::expected<lib::Buffer<VertexPT>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords) {
	if (positions.size() != texCoords.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPT> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), texCoords.cbegin(), vertices.begin(),
		[](const glm::vec3& pos, const glm::vec2& texCoord) { return VertexPT{ pos, texCoord }; });
	return vertices;
}

std::expected<lib::Buffer<VertexPTN>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTN> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
	}
	return vertices;
}

std::expected<lib::Buffer<VertexPTNT>, std::string_view> buildInterleavingVertexData(const std::vector<glm::vec3>& positions, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size() || positions.size() != tangents.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTNT> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = positions[i];
		vertices[i].texCoord = texCoords[i];
		vertices[i].normal = normals[i];
		vertices[i].tangent = tangents[i];
	}
	return vertices;
}