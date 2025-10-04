#pragma once

#include <algorithm>
#include <functional>
#include <future>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "common/file/file_loader.h"
#include "common/model_loader/image_loader/image_loader.h"
#include "common/status/status.h"
#include "common/util/asset_manager.h"
#include "common/util/geometry.h"
#include "common/util/primitives.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/util/index_buffer_util.h"

class AssetManager : public common::AssetManager<AssetManager> {
public:
  AssetManager() = default;

  AssetManager(const LogicalDevice& logicalDevice, const std::shared_ptr<FileLoader>& fileLoader);

  AssetManager& operator=(AssetManager&& assetManager) noexcept;

  ~AssetManager() = default;

  struct ImageData {
    Buffer stagingBuffer;
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;
    std::vector<ImageSubresource> copyRegions;
  };

  struct VertexData {
    Buffer vertexBuffer;
    Buffer indexBuffer;
    VkIndexType indexType;
    Buffer vertexBufferPositions;
  };

  void loadImageAsync(const std::string& filePath);

  void loadVertexDataInterleavingAsync(
      common::ModelPointer& modelPtr, const std::string& name, std::span<const std::byte> indices,
      uint8_t indexSize, std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
      std::span<const glm::vec3> normals);

  template <typename VertexType>
  void loadVertexDataAsync(const std::string& filePath, std::span<const std::byte> indices,
                           uint8_t indexSize, std::span<const VertexType> vertices);

  ErrorOr<std::reference_wrapper<const ImageData>> getImageData(const std::string& filePath);

  ErrorOr<std::reference_wrapper<const VertexData>> getVertexData(const std::string& filePath);

private:
  void loadImageAsync(
      const std::string& filePath,
      std::function<ErrorOr<ImageResource>(std::span<const std::byte>)>&& loadingFunction);

  const LogicalDevice* _logicalDevice = nullptr;

  std::shared_ptr<FileLoader> _fileLoader;

  std::unordered_map<std::string, VertexData> _vertexDataResources;
  std::unordered_map<std::string, std::future<ErrorOr<VertexData>>> _awaitingVertexDataResources;

  std::unordered_map<std::string, ImageData> _imageResources;
  std::unordered_map<std::string, std::future<ErrorOr<ImageData>>> _awaitingImageResources;
};

template <typename Type>
void AssetManager::loadVertexDataAsync(const std::string& name, std::span<const std::byte> indices,
                                       uint8_t indexSize, std::span<const Type> vertices) {
  if (_awaitingVertexDataResources.contains(name)) {
    return;
  }
  auto future = std::async(
      std::launch::async, ([this, indices, indexSize,
                            vertices]() -> ErrorOr<VertexData> {  // TODO: boost::asio::post,
                                                                  // boost::asio::use_future
        ASSIGN_OR_RETURN(auto vertexBuffer, Buffer::createStagingBuffer(
                                                *_logicalDevice, vertices.size() * sizeof(Type)));
        RETURN_IF_ERROR(vertexBuffer.copyData(vertices));
        ASSIGN_OR_RETURN(
            auto indexBuffer, Buffer::createStagingBuffer(*_logicalDevice, indices.size()));
        RETURN_IF_ERROR(indexBuffer.copyData(indices));
        return VertexData{
          Buffer(), std::move(indexBuffer), getIndexType(indexSize), std::move(vertexBuffer)};
      }));
  _awaitingVertexDataResources.emplace(name, std::move(future));
}
