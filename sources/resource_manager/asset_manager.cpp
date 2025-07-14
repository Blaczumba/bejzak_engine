#include "asset_manager.h"

#include <chrono>

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(LogicalDevice& logicalDevice) : _logicalDevice(logicalDevice) {}

void AssetManager::loadImageAsync(const std::string& filePath, std::function<ErrorOr<ImageResource>(std::string_view)>&& loadingFunction) {
    if (_awaitingImageResources.contains(filePath)) {
        return;
    }
    auto future = std::async(std::launch::async, ([this, filePath, loadingFunction = std::move(loadingFunction)]() {
        ErrorOr<ImageResource> resource = loadingFunction(filePath);
        if (!resource.has_value()) [[unlikely]]
            return ErrorOr<ImageData>(Error(resource.error()));
        auto stagingBuffer = Buffer::createStagingBuffer(_logicalDevice, resource->size);
        if (!stagingBuffer.has_value()) [[unlikely]]
            return ErrorOr<ImageData>(Error(stagingBuffer.error()));
        stagingBuffer->copyData(std::span(static_cast<const uint8_t*>(resource->data), resource->size));
        ImageLoader::deallocateResources(*resource);
        return ErrorOr<ImageData>(ImageData(std::move(*stagingBuffer), std::move(resource->dimensions)));
    }));
    _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImage2DAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::load2DImage);
}

void AssetManager::loadImageCubemapAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::loadCubemapImage);
}

void AssetManager::loadVertexDataInterleaving(const std::string& filePath, std::span<const uint8_t> indices, uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords, std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
    if (_awaitingVertexDataResources.contains(filePath)) {
        return;
    }
    auto future = std::async(std::launch::async, ([this, indices, indexSize, positions, texCoords, normals, tangents]() {
        auto vertexBuffer = Buffer::createStagingBuffer(_logicalDevice, positions.size() * sizeof(VertexPTNT));
        if (!vertexBuffer.has_value()) [[unlikely]] {
            return ErrorOr<VertexData>(Error(vertexBuffer.error()));
        }
        auto start = std::chrono::high_resolution_clock::now();
        if (Status copyStatus = vertexBuffer->copyDataInterleaving(positions, texCoords, normals, tangents); !copyStatus.has_value()) [[unlikely]] {
            return ErrorOr<VertexData>(Error(copyStatus.error()));
        }
        auto stop = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count() << std::endl << std::endl;
        auto vertexBufferPositions = Buffer::createStagingBuffer(_logicalDevice, positions.size() * sizeof(glm::vec3));
        if (!vertexBufferPositions.has_value()) [[unlikely]] {
            return ErrorOr<VertexData>(Error(vertexBufferPositions.error()));
        }
        if (Status copyStatus = vertexBufferPositions->copyData(positions); !copyStatus.has_value()) [[unlikely]] {
            return ErrorOr<VertexData>(Error(copyStatus.error()));
        }
        auto indexBuffer = Buffer::createStagingBuffer(_logicalDevice, indices.size());
        if (!indexBuffer.has_value()) [[unlikely]] {
            return ErrorOr<VertexData>(Error(indexBuffer.error()));
        }
        indexBuffer->copyData<uint8_t>(indices);
        return ErrorOr<VertexData>(VertexData(std::move(vertexBuffer.value()), std::move(indexBuffer.value()), getIndexType(indexSize), std::move(vertexBufferPositions.value()), AABB{}));
    }));
    _awaitingVertexDataResources.emplace(filePath, std::move(future));
}

ErrorOr<const ImageData*> AssetManager::getImageData(const std::string& filePath) {
    auto imageIt = _imageResources.find(filePath);
    if (imageIt != _imageResources.cend()) {
        return &imageIt->second;
    }
	auto it = _awaitingImageResources.find(filePath);
    if (it != _awaitingImageResources.cend()) {
        ASSIGN_OR_RETURN(ImageData imageData, it->second.get());
        auto ptr = _imageResources.emplace(filePath, std::move(imageData));
        _awaitingImageResources.erase(it);
		return &ptr.first->second;
    }
    return Error(EngineError::NOT_FOUND);
}

ErrorOr<const VertexData*> AssetManager::getVertexData(const std::string& filePath) {
    auto vertexIt = _vertexDataResources.find(filePath);
    if (vertexIt != _vertexDataResources.cend()) {
        return &vertexIt->second;
    }
    auto it = _awaitingVertexDataResources.find(filePath);
    if (it != _awaitingVertexDataResources.cend()) {
        ASSIGN_OR_RETURN(auto vertexData, it->second.get());
        auto ptr = _vertexDataResources.emplace(filePath, std::move(vertexData));
        _awaitingVertexDataResources.erase(it);
        return &ptr.first->second;
    }
    return Error(EngineError::NOT_FOUND);
}
