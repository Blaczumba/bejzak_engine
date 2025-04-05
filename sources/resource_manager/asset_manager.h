#pragma once

#include "lib/buffer/shared_buffer.h"
#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "memory_objects/staging_buffer.h"
#include "model_loader/image_loader/image_loader.h"
#include "primitives/geometry.h"
#include "primitives/primitives.h"
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
	AssetManager(MemoryAllocator& memoryAllocator);
	
	struct ImageData {
		std::unique_ptr<StagingBuffer> stagingBuffer;
		ImageDimensions imageDimensions;
	};

	struct VertexData {
		std::unique_ptr<StagingBuffer> vertexBuffer;
		std::unique_ptr<StagingBuffer> indexBuffer;
		VkIndexType indexType;
		std::unique_ptr<StagingBuffer> vertexBufferPrimitives;
		AABB aabb;
	};

	void loadImage2DAsync(const std::string& filePath);
	void loadImageCubemapAsync(const std::string& filePath);

	template<typename VertexType, typename PrimitiveType>
	void loadVertexData(const std::string& filePath, lib::SharedBuffer<uint8_t>& indices, uint8_t indexSize, lib::SharedBuffer<VertexType>& vertices, lib::SharedBuffer<PrimitiveType>& primitives);

	template<typename VertexType>
	void loadVertexData(const std::string& filePath, lib::SharedBuffer<uint8_t>& indices, uint8_t indexSize, lib::SharedBuffer<VertexType>& vertices);

	lib::ErrorOr<const ImageData*> getImageData(const std::string& filePath);
	lib::ErrorOr<const VertexData*> getVertexData(const std::string& filePath);

private:
	void loadImageAsync(const std::string& filePath, std::function<lib::ErrorOr<ImageResource>(std::string_view)>&& loadingFunction);

	MemoryAllocator& _memoryAllocator;

	std::unordered_map<std::string, VertexData> _vertexDataResources;
	std::unordered_map<std::string, std::future<lib::ErrorOr<VertexData>>> _awaitingVertexDataResources;

	std::unordered_map<std::string, ImageData> _imageResources;
	std::unordered_map<std::string, std::future<lib::ErrorOr<ImageData>>> _awaitingImageResources;
};

template<typename VertexType, typename PrimitiveType>
void AssetManager::loadVertexData(const std::string& filePath, lib::SharedBuffer<uint8_t>& indices, uint8_t indexSize, lib::SharedBuffer<VertexType>& vertices, lib::SharedBuffer<PrimitiveType>& primitives) {
	if (_awaitingVertexDataResources.contains(filePath)) {
		return;
	}
	auto future = std::async(std::launch::async, ([this, indices, indexSize, vertices, primitives]() {
		auto vertexBuffer = StagingBuffer::create(_memoryAllocator, vertices);
		if (!vertexBuffer.has_value()) [[unlikely]] {
			return lib::ErrorOr<VertexData>(lib::Error(vertexBuffer.error()));
		}
		auto vertexBufferPrimitives = StagingBuffer::create(_memoryAllocator, primitives);
		if (!vertexBufferPrimitives.has_value()) [[unlikely]] {
			return lib::ErrorOr<VertexData>(lib::Error(vertexBufferPrimitives.error()));
		}
		auto indexBuffer = StagingBuffer::create(_memoryAllocator, indices);
		if (!indexBuffer.has_value()) [[unlikely]] {
			return lib::ErrorOr<VertexData>(lib::Error(indexBuffer.error()));
		}
		return lib::ErrorOr<VertexData>(VertexData(std::move(vertexBuffer.value()), std::move(indexBuffer.value()), getIndexType(indexSize), std::move(vertexBufferPrimitives.value()), AABB{}));
	}));
	_awaitingVertexDataResources.emplace(filePath, std::move(future));
}

template<typename VertexType>
void AssetManager::loadVertexData(const std::string& filePath, lib::SharedBuffer<uint8_t>& indices, uint8_t indexSize, lib::SharedBuffer<VertexType>& vertices) {
	if (_awaitingVertexDataResources.contains(filePath)) {
		return;
	}
	auto future = std::async(std::launch::async, ([this, indices, indexSize, vertices]() {
		auto vertexBuffer = StagingBuffer::create(_memoryAllocator, vertices);
		if (!vertexBuffer.has_value()) [[unlikely]] {
			return lib::ErrorOr<VertexData>(lib::Error(vertexBuffer.error()));
		}
		auto indexBuffer = StagingBuffer::create(_memoryAllocator, indices);
		if (!indexBuffer.has_value()) [[unlikely]] {
			return lib::ErrorOr<VertexData>(lib::Error(indexBuffer.error()));
		}
		return lib::ErrorOr<VertexData>(VertexData(nullptr, std::move(indexBuffer.value()), getIndexType(indexSize), std::move(vertexBuffer.value()), AABB{}));
	}));
	_awaitingVertexDataResources.emplace(filePath, std::move(future));
}
