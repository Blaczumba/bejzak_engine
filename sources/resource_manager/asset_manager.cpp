#include "asset_manager.h"

using ImageData = AssetManager::ImageData;
using VertexData = AssetManager::VertexData;

AssetManager::AssetManager(MemoryAllocator& memoryAllocator) : _memoryAllocator(memoryAllocator) {}

void AssetManager::loadImageAsync(const std::string& filePath, std::function<lib::ErrorOr<ImageResource>(std::string_view)>&& loadingFunction) {
    auto it = _awaitingImageResources.find(filePath);
    if (it != _awaitingImageResources.end()) {
        return;
    }

    auto future = std::async(std::launch::async, ([this, filePath, loadingFunction = std::move(loadingFunction)]() {
        lib::ErrorOr<ImageResource> resource = loadingFunction(filePath);
        if (!resource.has_value())
            return lib::ErrorOr<ImageData>(lib::Error(std::move(resource.error())));
        StagingBuffer stagingBuffer(_memoryAllocator, std::span(static_cast<uint8_t*>(resource->data), resource->size));
        ImageLoader::deallocateResources(*resource);
        return lib::ErrorOr<ImageData>(ImageData(std::move(stagingBuffer), std::move(resource->dimensions)));
    }));
    _awaitingImageResources.emplace(filePath, std::move(future));
}

void AssetManager::loadImage2DAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::load2DImage);
}

void AssetManager::loadImageCubemapAsync(const std::string& filePath) {
	loadImageAsync(filePath, ImageLoader::loadCubemapImage);
}

lib::ErrorOr<const ImageData*> AssetManager::getImageData(const std::string& filePath) {
    auto imageIt = _imageResources.find(filePath);
    if (imageIt != _imageResources.cend()) {
        return &imageIt->second;
    }
	auto it = _awaitingImageResources.find(filePath);
    if (it != _awaitingImageResources.cend()) {
        ASSIGN_OR_RETURN(auto imageData, it->second.get());
        auto ptr = _imageResources.emplace(filePath, std::move(imageData));
        _awaitingImageResources.erase(it);
		return &ptr.first->second;
    }
    return lib::Error("Image data not found");
}

void AssetManager::deleteImage(std::string_view filePath) {

	return;
}
