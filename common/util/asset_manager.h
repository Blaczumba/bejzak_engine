#pragma once

#include <glm/glm.hpp>
#include <span>
#include <string>

namespace common {

template <typename AssetManagerImpl>
class AssetManager {
public:
  void loadImageAsync(const std::string& filePath) {
    static_cast<AssetManagerImpl*>(this)->loadImageAsync(filePath);
  }

  void loadVertexDataInterleavingAsync(
      const std::string& name, std::span<const std::byte> indices, uint8_t indexSize,
      std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
      std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
    static_cast<AssetManagerImpl*>(this)->loadVertexDataInterleavingAsync(
        name, indices, indexSize, positions, texCoords, normals, tangents);
  }

  template <typename VertexType>
  void loadVertexDataAsync(const std::string& filePath, std::span<const std::byte> indices,
                           uint8_t indexSize, std::span<const VertexType> vertices) {
    static_cast<AssetManagerImpl*>(this)->template loadVertexDataAsync<VertexType>(
        filePath, indices, indexSize, vertices);
  }
};

}  // namespace common
