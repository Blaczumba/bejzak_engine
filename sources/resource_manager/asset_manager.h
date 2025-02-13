#pragma once

#include "lib/buffer/buffer.h"
#include "logical_device/logical_device.h"
#include "memory_objects/staging_buffer.h"
#include "model_loader/image_loader/image_loader.h"
#include "primitives/geometry.h"
#include "primitives/primitives.h"
#include "thread_pool/thread_pool.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <unordered_set>

enum class CacheCode : uint8_t {
	NOT_CACHED,
	NOT_YET_PROCESSED,
	CACHED
};

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
	AssetManager(MemoryAllocator& memoryAllocator);
	
	struct ImageData {
		StagingBuffer stagingBuffer;
		ImageDimensions imageDimensions;
	};

	struct VertexData {
		std::optional<StagingBuffer> vertexBuffer;	// std::nullopt when the type is VertexP.
		StagingBuffer indexBuffer;
		VkIndexType indexType;
		StagingBuffer vertexBufferPrimitives;	// VertexP/glm::vec3 buffer.
		AABB aabb;
	};

	std::shared_future<std::unique_ptr<VertexData>> loadVertexDataAsync(const std::string& identifier, std::function<ImageResource()>&& loadingFunction);
	void loadImage2DAsync(const std::string& filePath);
	void loadImageCubemapAsync(const std::string& filePath);

	template<typename VertexType>
	CacheCode loadVertexData(std::string_view key, const std::vector<VertexType>& vertices, const lib::Buffer<uint8_t>& indices, uint8_t indexSize) {
		// TODO: Needs refactoring
		static_assert(VertexTraits<VertexType>::hasPosition, "Cannot load vertex data with no position defined");
		auto createStagingBuffer = [&](const auto& data) {
			return StagingBuffer(_memoryAllocator, data);
		};
		StagingBuffer indexBuffer = createStagingBuffer(indices);
		auto handleVertexBuffer = [&]() -> std::tuple<std::optional<StagingBuffer>, StagingBuffer> {
			if (typeid(VertexType) == typeid(VertexP)) {
				return { std::nullopt, createStagingBuffer(std::span(vertices.data(), vertices.size()))};
			}
			else {
				std::vector<glm::vec3> primitives;
				primitives.reserve(vertices.size());
				std::transform(vertices.begin(), vertices.end(), std::back_inserter(primitives), [](const VertexType& vertex) { return vertex.pos; });
				return { createStagingBuffer(std::span(vertices.data(), vertices.size())), createStagingBuffer(std::span(primitives.data(), primitives.size())) };
			}
		};
		auto [vertexBuffer, primitivesVertexBuffer] = handleVertexBuffer();
		_vertexDataResources.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(key),
			std::forward_as_tuple(
				std::move(vertexBuffer),
				std::move(indexBuffer),
				getIndexType(indexSize),
				std::move(primitivesVertexBuffer),
				AABB{}
			)
		);

		return CacheCode::NOT_CACHED;
	}

	const ImageData& getImageData(const std::string& filePath);
	void waitForImagesToLoad() const;
	void deleteImage(std::string_view filePath);

	const VertexData& getVertexData(std::string_view key) const {
		auto ptr = _vertexDataResources.find(std::string{ key });
		return ptr->second; // TODO if not found branch
	}

private:
	void loadImageAsync(const std::string& filePath, std::function<ImageResource(std::string_view)>&& loadingFunction);

	MemoryAllocator& _memoryAllocator;

	std::unordered_map<std::string, VertexData> _vertexDataResources;
	std::unordered_map<std::string, std::shared_future<std::unique_ptr<VertexData>>> _awaitingVertexDataResources;

	std::unordered_map<std::string, ImageData> _imageResources;
	std::unordered_map<std::string, std::future<std::unique_ptr<ImageData>>> _awaitingImageResources;
};
