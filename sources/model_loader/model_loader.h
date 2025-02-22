#pragma once

#include "lib/buffer/buffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <string>
#include <vector>

enum class IndexType : uint8_t {
	NONE = 0,
	UINT8 = 1,
	UINT16 = 2,
	UINT32 = 4
};

IndexType getMatchingIndexType(size_t indicesCount);

template<typename IndexT>
std::enable_if_t<std::is_unsigned<IndexT>::value> processIndices(uint8_t* dstIndices, const IndexT* srcIndices, size_t indicesCount, IndexType indexType) {
	size_t offset = {};
	switch (indexType) {
	case IndexType::UINT8:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexT& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint8_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
		break;
	case IndexType::UINT16:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexT& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint16_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
		break;
	case IndexType::UINT32:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexT& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint32_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
	}
}

template<typename VertexType>
struct VertexData {
	std::vector<VertexType> vertices;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textureCoordinates;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangents;
	lib::Buffer<uint8_t> indices;
	IndexType indexType;

	glm::mat4 model;

	std::string diffuseTexture;
	std::string normalTexture;
	std::string metallicRoughnessTexture;

};

template<typename VertexType>
class ModelLoader {
public:
	~ModelLoader() = default;
};