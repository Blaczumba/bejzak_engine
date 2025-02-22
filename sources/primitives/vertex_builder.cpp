#include "vertex_builder.h"

constexpr std::string_view NOT_MATCHING_SIZES = "Sizes of input data do not match.";
constexpr std::string_view EMPTY_INPUT_DATA = "Input data is empty.";

std::expected<lib::Buffer<VertexP>, std::string_view> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions) {
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	return lib::Buffer<VertexP>(positions.size());
}

std::expected<lib::Buffer<VertexPT>, std::string_view> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords) {
	if (positions.size() != texCoords.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPT> vertices(positions.size());
	std::transform(positions.cbegin(), positions.cend(), texCoords.cbegin(), vertices.begin(),
		[](const glm::vec3& pos, const glm::vec2& texCoord) { return VertexPT{ pos, texCoord }; });
	return vertices;
}

std::expected<lib::Buffer<VertexPTN>, std::string_view> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTN> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i] = VertexPTN{ positions[i], texCoords[i], normals[i] };
	}
	return vertices;
}

std::expected<lib::Buffer<VertexPTNT>, std::string_view> buildInterleavingVertexData(const lib::Buffer<glm::vec3>& positions, const lib::Buffer<glm::vec2>& texCoords, const lib::Buffer<glm::vec3>& normals, const lib::Buffer<glm::vec3>& tangents) {
	if (positions.size() != texCoords.size() || positions.size() != normals.size() || positions.size() != tangents.size())
		return std::unexpected(NOT_MATCHING_SIZES);
	if (positions.size() == 0)
		return std::unexpected(EMPTY_INPUT_DATA);
	lib::Buffer<VertexPTNT> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i] = VertexPTNT{ positions[i], texCoords[i], normals[i], tangents[i] };
	}
	return vertices;
}