#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
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
		std::unique_ptr<StagingBuffer> stagingBuffer;
		ImageDimensions imageDimensions;
	};

	struct VertexData {
		std::unique_ptr<StagingBuffer> vertexBuffer;
		std::unique_ptr<StagingBuffer> indexBuffer;
		VkIndexType indexType;
		std::unique_ptr<StagingBuffer> vertexBufferPrimitives;	// VertexP/glm::vec3 buffer.
		AABB aabb;
	};

	void loadImage2DAsync(const std::string& filePath);
	void loadImageCubemapAsync(const std::string& filePath);

	template<BufferLike IndexBuffer, BufferLike FirstBuffer, BufferLike SecondBuffer>
	lib::Status loadVertexData(std::string_view key, const IndexBuffer& indices, uint8_t indexSize, const FirstBuffer& vertices, const SecondBuffer& primitives) {
		ASSIGN_OR_RETURN(auto indexBuffer, StagingBuffer::create(_memoryAllocator, indices));
		ASSIGN_OR_RETURN(auto vertexBuffer, StagingBuffer::create(_memoryAllocator, vertices));
		ASSIGN_OR_RETURN(auto primitivesVertexBuffer, StagingBuffer::create(_memoryAllocator, primitives));
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
		return lib::StatusOk();
	}

	template<BufferLike IndexBuffer, BufferLike Buffer>
	lib::Status loadVertexData(std::string_view key, const IndexBuffer& indices, uint8_t indexSize, const Buffer& vertices) {
		ASSIGN_OR_RETURN(auto vertexBuffer, StagingBuffer::create(_memoryAllocator, vertices));
		ASSIGN_OR_RETURN(auto indexBuffer, StagingBuffer::create(_memoryAllocator, indices));
		_vertexDataResources.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(key),
			std::forward_as_tuple(
				nullptr,
				std::move(indexBuffer),
				getIndexType(indexSize),
				std::move(vertexBuffer),
				AABB{}
			)
		);
		return lib::StatusOk();
	}

	lib::ErrorOr<const ImageData*> getImageData(const std::string& filePath);
	void deleteImage(std::string_view filePath);

	const VertexData& getVertexData(std::string_view key) const {
		auto ptr = _vertexDataResources.find(std::string{ key });
		return ptr->second; // TODO if not found branch
	}

private:
	void loadImageAsync(const std::string& filePath, std::function<lib::ErrorOr<ImageResource>(std::string_view)>&& loadingFunction);

	MemoryAllocator& _memoryAllocator;

	std::unordered_map<std::string, VertexData> _vertexDataResources;
	std::unordered_map<std::string, std::future<std::unique_ptr<VertexData>>> _awaitingVertexDataResources;

	std::unordered_map<std::string, ImageData> _imageResources;
	std::unordered_map<std::string, std::future<lib::ErrorOr<ImageData>>> _awaitingImageResources;
};
