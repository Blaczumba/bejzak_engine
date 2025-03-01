#include "resource_manager.h"

#include "memory_objects/texture/image.h"
#include "memory_objects/vertex_buffer.h"

ResourceManager::ResourceManager(const LogicalDevice& logicalDevice)
    : _logicalDevice(logicalDevice) {}

Texture* ResourceManager::getTexture(std::string_view filePath) {
    auto it = _textures.find(std::string{ filePath });
    return it == _textures.cend() ? nullptr : &it->second;
}
