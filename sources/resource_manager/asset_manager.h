#pragma once

#include "lib/buffer/shared_buffer.h"
#include "logical_device/logical_device.h"
#include "memory_objects/buffer.h"
#include "model_loader/image_loader/image_loader.h"
#include "primitives/geometry.h"
#include "primitives/primitives.h"
#include "status/status.h"
#include "thread_pool/thread_pool.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace {

constexpr VkIndexType getIndexType(uint8_t indexSize) {
	switch (indexSize) {
	case 1:
		return VK_INDEX_TYPE_UINT8_EXT;
	case 2:
		return VK_INDEX_TYPE_UINT16;
	case 4:
		return VK_INDEX_TYPE_UINT32;
	default:
		return VK_INDEX_TYPE_NONE_KHR;
	}
}

}

class AssetManager {
public:
	AssetManager(LogicalDevice& logicalDevice);
	
	struct ImageData {
		Buffer stagingBuffer;
		ImageDimensions imageDimensions;
	};

	struct VertexData {
		Buffer vertexBuffer;
		Buffer indexBuffer;
		VkIndexType indexType;
		Buffer vertexBufferPrimitives;
		AABB aabb;
	};

	void loadImage2DAsync(const std::string& filePath);
	void loadImageCubemapAsync(const std::string& filePath);

	void loadVertexDataInterleavingAsync(const std::string& name, std::span<const uint8_t> indices, uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents);

	template<typename VertexType>
	void loadVertexDataAsync(const std::string& filePath, std::span<const uint8_t> indices, uint8_t indexSize, std::span<const VertexType> vertices);

	ErrorOr<const ImageData*> getImageData(const std::string& filePath);
	ErrorOr<const VertexData*> getVertexData(const std::string& filePath);

private:
	void loadImageAsync(const std::string& filePath, std::function<ErrorOr<ImageResource>(std::string_view)>&& loadingFunction);

	LogicalDevice& _logicalDevice;

	std::unordered_map<std::string, VertexData> _vertexDataResources;
	std::unordered_map<std::string, std::future<ErrorOr<VertexData>>> _awaitingVertexDataResources;

	std::unordered_map<std::string, ImageData> _imageResources;
	std::unordered_map<std::string, std::future<ErrorOr<ImageData>>> _awaitingImageResources;
};

template<typename Type>
void AssetManager::loadVertexDataAsync(const std::string& filePath, std::span<const uint8_t> indices, uint8_t indexSize, std::span<const Type> vertices) {
	if (_awaitingVertexDataResources.contains(filePath)) {
		return;
	}
	auto future = std::async(std::launch::async, ([this, indices, indexSize, vertices]() {
		auto vertexBuffer = Buffer::createStagingBuffer(_logicalDevice, vertices.size() * sizeof(Type));
		if (!vertexBuffer.has_value()) [[unlikely]] {
			return ErrorOr<VertexData>(Error(vertexBuffer.error()));
		}
		vertexBuffer->copyData<Type>(vertices);
		auto indexBuffer = Buffer::createStagingBuffer(_logicalDevice, indices.size());
		if (!indexBuffer.has_value()) [[unlikely]] {
			return ErrorOr<VertexData>(Error(indexBuffer.error()));
		}
		indexBuffer->copyData<uint8_t>(indices);
		return ErrorOr<VertexData>(VertexData{ Buffer(), std::move(indexBuffer.value()), getIndexType(indexSize), std::move(vertexBuffer.value()), AABB{} });
	}));
	_awaitingVertexDataResources.emplace(filePath, std::move(future));
}
