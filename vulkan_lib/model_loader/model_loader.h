#pragma once

#include "vulkan_lib/lib/buffer/shared_buffer.h"

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
std::enable_if_t<std::is_unsigned<IndexT>::value, lib::Buffer<uint8_t>> processIndices(std::span<const IndexT> srcIndices, IndexType indexType) {
	lib::Buffer<uint8_t> indices(srcIndices.size() * static_cast<size_t>(indexType));
	uint8_t* data = indices.data();
	for (const IndexT& index : srcIndices) {
		std::memcpy(data, &index, static_cast<size_t>(indexType));
		data += static_cast<size_t>(indexType);
	}
	return indices;
}

struct VertexData {
	lib::SharedBuffer<glm::vec3> positions;
	lib::SharedBuffer<glm::vec2> textureCoordinates;
	lib::SharedBuffer<glm::vec3> normals;
	lib::SharedBuffer<glm::vec3> tangents;
	lib::SharedBuffer<uint8_t> indices;
	IndexType indexType;

	glm::mat4 model;

	std::string diffuseTexture;
	std::string normalTexture;
	std::string metallicRoughnessTexture;

};

class ModelLoader {
public:
	~ModelLoader() = default;
};