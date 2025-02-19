#pragma once

#include "lib/buffer/buffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <string>
#include <vector>

enum class IndexTypeT : uint8_t {
	NONE = 0,
	UINT8 = 1,
	UINT16 = 2,
	UINT32 = 4
};

IndexTypeT getMatchingIndexType(size_t indicesCount);

template<typename IndexType>
std::enable_if_t<std::is_unsigned<IndexType>::value> processIndices(uint8_t* dstIndices, const IndexType* srcIndices, size_t indicesCount, IndexTypeT indexType) {
	size_t offset = {};
	switch (indexType) {
	case IndexTypeT::UINT8:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexType& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint8_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
		break;
	case IndexTypeT::UINT16:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexType& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint16_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
		break;
	case IndexTypeT::UINT32:
		std::for_each(srcIndices, srcIndices + indicesCount, [=, &offset](const IndexType& srcIndex) {std::memcpy(dstIndices + offset, &static_cast<const uint32_t&>(srcIndex), static_cast<size_t>(indexType)); offset += static_cast<size_t>(indexType); });
	}
}

template<typename VertexType>
struct VertexData {
	std::vector<VertexType> vertices;
	std::string diffuseTexture;
	std::string normalTexture;
	std::string metallicRoughnessTexture;
	glm::mat4 model;

	lib::Buffer<uint8_t> indices;
	IndexTypeT indexType;
};

template<typename VertexType>
class ModelLoader {
public:
	~ModelLoader() = default;
};