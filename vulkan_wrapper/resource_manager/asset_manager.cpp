#include "asset_manager.h"

#include <chrono>

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(const std::shared_ptr<FileLoader>& fileLoader)
  : _fileLoader(fileLoader) {}

void AssetManager::loadImageAsync(
    LogicalDevice& logicalDevice, const std::string& filePath,
    std::function<ErrorOr<ImageResource>(std::span<const std::byte>)>&& loadingFunction) {
  if (_awaitingImageResources.contains(filePath)) {
    return;
  }
  auto future = std::async(
      std::launch::async,
      [this, logicalDevice = std::ref(logicalDevice), filePath,
       loadingFunction = std::move(loadingFunction)]() -> ErrorOr<ImageData> {  // TODO:
        // boost::asio::post,
        // boost::asio::use_future
        ASSIGN_OR_RETURN(lib::Buffer<std::byte> fileData, _fileLoader->loadFileToBuffer(filePath));
        ASSIGN_OR_RETURN(ImageResource resource, loadingFunction(fileData));
        ASSIGN_OR_RETURN(
            auto stagingBuffer, Buffer::createStagingBuffer(logicalDevice, resource.size));
        RETURN_IF_ERROR(stagingBuffer.copyData(
            std::span(static_cast<const std::byte*>(resource.data), resource.size)));
        ImageLoader::deallocateResources(resource);
        return ImageData(std::move(stagingBuffer), resource.width, resource.height,
                         resource.mipLevels, resource.layerCount, std::move(resource.subresources));
      });
  _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImageAsync(LogicalDevice& logicalDevice, const std::string& filePath) {
  if (filePath.ends_with(".ktx") || filePath.ends_with(".ktx2")) {
    loadImageAsync(logicalDevice, filePath, ImageLoader::loadImageKtx);
  } else {
    loadImageAsync(logicalDevice, filePath, ImageLoader::loadImageStbi);
  }
}

void AssetManager::loadVertexDataInterleavingAsync(
    LogicalDevice& logicalDevice, const std::string& name, std::span<const std::byte> indices,
    uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
  if (_awaitingVertexDataResources.contains(name)) {
    return;
  }
  auto future = std::async(
      std::launch::async,
      [this, logicalDevice = std::ref(logicalDevice), indices, indexSize, positions, texCoords,
       normals, tangents]() -> ErrorOr<VertexData> {  // TODO: boost::asio::post,
                                                      // boost::asio::use_future
        ASSIGN_OR_RETURN(
            auto vertexBuffer,
            Buffer::createStagingBuffer(logicalDevice, positions.size() * sizeof(VertexPTNT)));
        RETURN_IF_ERROR(vertexBuffer.copyDataInterleaving(positions, texCoords, normals, tangents));
        ASSIGN_OR_RETURN(
            auto vertexBufferPositions,
            Buffer::createStagingBuffer(logicalDevice, positions.size() * sizeof(glm::vec3)));
        RETURN_IF_ERROR(vertexBufferPositions.copyData(positions));
        ASSIGN_OR_RETURN(
            auto indexBuffer, Buffer::createStagingBuffer(logicalDevice, indices.size()));
        RETURN_IF_ERROR(indexBuffer.copyData(indices));
        return ErrorOr<VertexData>(
            VertexData(std::move(vertexBuffer), std::move(indexBuffer), getIndexType(indexSize),
                       std::move(vertexBufferPositions)));
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
