#include "asset_manager.h"

#include <chrono>

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(std::unique_ptr<FileLoader>&& fileLoader)
  : _fileLoader(std::move(fileLoader)) {}

void AssetManager::loadImageAsync(
    LogicalDevice& logicalDevice, const std::string& filePath,
    std::function<ErrorOr<ImageResource>(std::span<const std::byte>)>&& loadingFunction) {
  if (_awaitingImageResources.contains(filePath)) {
    return;
  }
  auto future =
      std::async(std::launch::async, [this, logicalDevice = std::ref(logicalDevice), filePath,
                                      loadingFunction = std::move(loadingFunction)]() {  // TODO:
        // boost::asio::post,
        // boost::asio::use_future
        ErrorOr<lib::Buffer<std::byte>> fileData = _fileLoader->loadFile(filePath);
        if (!fileData.has_value()) [[unlikely]] {
          return ErrorOr<ImageData>(Error(fileData.error()));
        }

        ErrorOr<ImageResource> resource = loadingFunction(*fileData);
        if (!resource.has_value()) [[unlikely]] {
          return ErrorOr<ImageData>(Error(resource.error()));
        }

        auto stagingBuffer = Buffer::createStagingBuffer(logicalDevice, resource->size);
        if (!stagingBuffer.has_value()) [[unlikely]] {
          return ErrorOr<ImageData>(Error(stagingBuffer.error()));
        }

        stagingBuffer->copyData(
            std::span(static_cast<const std::byte*>(resource->data), resource->size));
        ImageLoader::deallocateResources(*resource);
        return ErrorOr<ImageData>(
            ImageData(std::move(*stagingBuffer), std::move(resource->dimensions)));
      });
  _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImage2DAsync(LogicalDevice& logicalDevice, const std::string& filePath) {
  loadImageAsync(logicalDevice, filePath, ImageLoader::load2DImage);
}

void AssetManager::loadImageCubemapAsync(
    LogicalDevice& logicalDevice, const std::string& filePath) {
  loadImageAsync(logicalDevice, filePath, ImageLoader::loadCubemapImage);
}

void AssetManager::loadVertexDataInterleavingAsync(
    LogicalDevice& logicalDevice, const std::string& name, std::span<const std::byte> indices,
    uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
  if (_awaitingVertexDataResources.contains(name)) {
    return;
  }
  auto future = std::async(
      std::launch::async, [this, logicalDevice = std::ref(logicalDevice), indices, indexSize,
                           positions, texCoords, normals, tangents]() {  // TODO: boost::asio::post,
                                                                         // boost::asio::use_future
        auto vertexBuffer =
            Buffer::createStagingBuffer(logicalDevice, positions.size() * sizeof(VertexPTNT));
        if (!vertexBuffer.has_value()) [[unlikely]] {
          return ErrorOr<VertexData>(Error(vertexBuffer.error()));
        }
        if (Status copyStatus =
                vertexBuffer->copyDataInterleaving(positions, texCoords, normals, tangents);
            !copyStatus.has_value()) [[unlikely]] {
          return ErrorOr<VertexData>(Error(copyStatus.error()));
        }
        auto vertexBufferPositions =
            Buffer::createStagingBuffer(logicalDevice, positions.size() * sizeof(glm::vec3));
        if (!vertexBufferPositions.has_value()) [[unlikely]] {
          return ErrorOr<VertexData>(Error(vertexBufferPositions.error()));
        }
        if (Status copyStatus = vertexBufferPositions->copyData(positions); !copyStatus.has_value())
            [[unlikely]] {
          return ErrorOr<VertexData>(Error(copyStatus.error()));
        }
        auto indexBuffer = Buffer::createStagingBuffer(logicalDevice, indices.size());
        if (!indexBuffer.has_value()) [[unlikely]] {
          return ErrorOr<VertexData>(Error(indexBuffer.error()));
        }
        if (Status copyStatus = indexBuffer->copyData(indices); !copyStatus.has_value())
            [[unlikely]] {
          return ErrorOr<VertexData>(Error(copyStatus.error()));
        }
        return ErrorOr<VertexData>(
            VertexData(std::move(vertexBuffer.value()), std::move(indexBuffer.value()),
                       getIndexType(indexSize), std::move(vertexBufferPositions.value())));
      });
  _awaitingVertexDataResources.emplace(name, std::move(future));
}

ErrorOr<std::reference_wrapper<const ImageData>> AssetManager::getImageData(
    const std::string& filePath) {
  auto imageIt = _imageResources.find(filePath);
  if (imageIt != _imageResources.cend()) {
    return imageIt->second;
  }
  auto it = _awaitingImageResources.find(filePath);
  if (it != _awaitingImageResources.cend()) {
    ASSIGN_OR_RETURN(ImageData imageData, it->second.get());
    auto ptr = _imageResources.emplace(filePath, std::move(imageData));
    _awaitingImageResources.erase(it);
    return ptr.first->second;
  }
  return Error(EngineError::NOT_FOUND);
}

ErrorOr<std::reference_wrapper<const VertexData>> AssetManager::getVertexData(
    const std::string& filePath) {
  auto vertexIt = _vertexDataResources.find(filePath);
  if (vertexIt != _vertexDataResources.cend()) {
    return vertexIt->second;
  }
  auto it = _awaitingVertexDataResources.find(filePath);
  if (it != _awaitingVertexDataResources.cend()) {
    ASSIGN_OR_RETURN(auto vertexData, it->second.get());
    auto ptr = _vertexDataResources.emplace(filePath, std::move(vertexData));
    _awaitingVertexDataResources.erase(it);
    return ptr.first->second;
  }
  return Error(EngineError::NOT_FOUND);
}
