#include "asset_manager.h"

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(MemoryAllocator& memoryAllocator) : _memoryAllocator(memoryAllocator) {}

void AssetManager::loadImageAsync(const std::string& filePath, std::function<ImageResource(std::string_view)>&& loadingFunction) {
    auto it = _awaitingImageResources.find(filePath);
    if (it != _awaitingImageResources.end()) {
        return;
    }

    auto future = std::async(std::launch::async, ([this, filePath, loadingFunction = std::move(loadingFunction)]() {
        ImageResource resource = loadingFunction(filePath);
        StagingBuffer stagingBuffer(_memoryAllocator, std::span(static_cast<uint8_t*>(resource.data), resource.size));
        ImageLoader::deallocateResources(resource);
        return std::make_unique<ImageData>(std::move(stagingBuffer), std::move(resource.dimensions));
    }));
    _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImage2DAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::load2DImage);
}

void AssetManager::loadImageCubemapAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::loadCubemapImage);
}

const ImageData& AssetManager::getImageData(const std::string& filePath) {
    // TODO return pointer
    auto imageIt = _imageResources.find(filePath);
    if (imageIt != _imageResources.cend()) {
        return imageIt->second;
    }
	auto it = _awaitingImageResources.find(filePath);
    if (it != _awaitingImageResources.cend()) {
        auto ptr = _imageResources.emplace(filePath, std::move(*it->second.get()));
        _awaitingImageResources.erase(it);
		return ptr.first->second;
    }
	throw std::runtime_error("Image data not found");
}

void AssetManager::deleteImage(std::string_view filePath) {

	return;
}
