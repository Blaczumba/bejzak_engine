#include "asset_manager.h"

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(const LogicalDevice& logicalDevice,
                           const std::shared_ptr<FileLoader>& fileLoader, std::launch launchPolicy)
  : _logicalDevice(&logicalDevice), _fileLoader(fileLoader), _launchPolicy(launchPolicy) {}

AssetManager& AssetManager::operator=(AssetManager&& assetManager) noexcept {
  if (this == &assetManager) {
    return *this;
  }

  _logicalDevice = std::exchange(assetManager._logicalDevice, nullptr);
  _fileLoader = std::move(assetManager._fileLoader);
  _vertexDataResources = std::move(assetManager._vertexDataResources);
  _awaitingVertexDataResources = std::move(assetManager._awaitingVertexDataResources);
  _imageResources = std::move(assetManager._imageResources);
  _awaitingImageResources = std::move(assetManager._awaitingImageResources);
  return *this;
}

void AssetManager::loadImageAsync(
    const std::string& filePath,
    std::function<ErrorOr<ImageResource>(std::span<const std::byte>)>&& loadingFunction) {
  if (_awaitingImageResources.contains(filePath)) {
    return;
  }
  auto future = std::async(
      _launchPolicy,
      [this, filePath,
       loadingFunction = std::move(loadingFunction)]() -> ErrorOr<ImageData> {  // TODO:
        // boost::asio::post,
        // boost::asio::use_future
        ASSIGN_OR_RETURN(lib::Buffer<std::byte> fileData, _fileLoader->loadFileToBuffer(filePath));
        ASSIGN_OR_RETURN(ImageResource resource, loadingFunction(fileData));
        ASSIGN_OR_RETURN(
            auto stagingBuffer, Buffer::createStagingBuffer(*_logicalDevice, resource.size));
        RETURN_IF_ERROR(stagingBuffer.copyData(
            std::span(static_cast<const std::byte*>(resource.data), resource.size)));
        ImageLoader::deallocateResources(resource);
        return ImageData(std::move(stagingBuffer), resource.width, resource.height,
                         resource.mipLevels, resource.layerCount, std::move(resource.subresources));
      });
  _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImageAsync(const std::string& filePath) {
  if (filePath.ends_with(".ktx") || filePath.ends_with(".ktx2")) {
    loadImageAsync(filePath, ImageLoader::loadImageKtx);
  } else {
    loadImageAsync(filePath, ImageLoader::loadImageStbi);
  }
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
