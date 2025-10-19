#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <string>

namespace common {

template <typename AssetManagerImpl>
class AssetManager {
public:
  void loadImageAsync(const std::string& filePath) {
    static_cast<AssetManagerImpl*>(this)->loadImageAsync(filePath);
  }

  template <typename Model>
  void loadVertexDataInterleavingAsync(
      std::shared_ptr<Model>& modelPtr, const std::string& name, std::span<const std::byte> indices,
      uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
      std::span<const glm::vec3> normals) {
    static_cast<AssetManagerImpl*>(this)->loadVertexDataInterleavingAsync(
        modelPtr, name, indices, indexSize, positions, texCoords, normals);
  }

  template <typename VertexType, typename Model>
  void loadVertexDataAsync(
      std::shared_ptr<Model>& modelPtr, const std::string& filePath,
      std::span<const std::byte> indices, uint8_t indexSize, std::span<const VertexType> vertices) {
    static_cast<AssetManagerImpl*>(this)->template loadVertexDataAsync<VertexType>(
        modelPtr, filePath, indices, indexSize, vertices);
  }
};

}  // namespace common
