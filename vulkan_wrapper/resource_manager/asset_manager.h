#pragma once

#include "common/status/status.h"
#include "common/util/geometry.h"
#include "common/util/primitives.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/model_loader/image_loader/image_loader.h"
#include "vulkan_wrapper/util/index_buffer_util.h"

#include <algorithm>
#include <future>
#include <optional>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <unordered_set>

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
		Buffer vertexBufferPositions;
	};

	void loadImage2DAsync(const std::string& filePath);
	void loadImageCubemapAsync(const std::string& filePath);

	void loadVertexDataInterleavingAsync(const std::string& name, std::span<const std::byte> indices, uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents);

	template<typename VertexType>
	void loadVertexDataAsync(const std::string& filePath, std::span<const std::byte> indices, uint8_t indexSize, std::span<const VertexType> vertices);

	ErrorOr<std::reference_wrapper<const ImageData>> getImageData(const std::string& filePath);
	ErrorOr<std::reference_wrapper<const VertexData>> getVertexData(const std::string& filePath);

private:
	void loadImageAsync(const std::string& filePath, std::function<ErrorOr<ImageResource>(std::string_view)>&& loadingFunction);

	LogicalDevice& _logicalDevice;

	std::unordered_map<std::string, VertexData> _vertexDataResources;
	std::unordered_map<std::string, std::future<ErrorOr<VertexData>>> _awaitingVertexDataResources;

	std::unordered_map<std::string, ImageData> _imageResources;
	std::unordered_map<std::string, std::future<ErrorOr<ImageData>>> _awaitingImageResources;
};

template<typename Type>
void AssetManager::loadVertexDataAsync(const std::string& filePath, std::span<const std::byte> indices, uint8_t indexSize, std::span<const Type> vertices) {
	if (_awaitingVertexDataResources.contains(filePath)) {
		return;
	}
	auto future = std::async(std::launch::async, ([this, indices, indexSize, vertices]() { // TODO: boost::asio::post, boost::asio::use_future
		auto vertexBuffer = Buffer::createStagingBuffer(_logicalDevice, vertices.size() * sizeof(Type));
		if (!vertexBuffer.has_value()) [[unlikely]] {
			return ErrorOr<VertexData>(Error(vertexBuffer.error()));
		}
		vertexBuffer->copyData(vertices);
		auto indexBuffer = Buffer::createStagingBuffer(_logicalDevice, indices.size());
		if (!indexBuffer.has_value()) [[unlikely]] {
			return ErrorOr<VertexData>(Error(indexBuffer.error()));
		}
		indexBuffer->copyData(indices);
		return ErrorOr<VertexData>(VertexData{ Buffer(), std::move(indexBuffer.value()), getIndexType(indexSize), std::move(vertexBuffer.value())});
	}));
	_awaitingVertexDataResources.emplace(filePath, std::move(future));
}
