#pragma once

#include <algorithm>
#include <functional>
#include <future>
#include <numeric>
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

  AssetManager(const LogicalDevice& logicalDevice, const std::shared_ptr<FileLoader>& fileLoader,
               std::launch launchPolicy = std::launch::async);

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
    std::unordered_map<std::string, Buffer> buffers;
    Buffer indexBuffer;
    VkIndexType indexType;
  };

  void loadImageAsync(const std::string& filePath);

  template <typename Model, typename... Type>
  void loadVertexDataInterleavingAsync(
      std::shared_ptr<Model>& modelPtr, const std::string& name, std::span<const std::byte> indices,
      uint8_t indexSize, std::span<const std::pair<std::string, std::string>> orderss,
      std::span<const Type>... attributes);

  template <typename VertexType, typename Model>
  void loadVertexDataAsync(
      std::shared_ptr<Model>& modelPtr, const std::string& filePath,
      std::span<const std::byte> indices, uint8_t indexSize, std::span<const VertexType> vertices);

  ErrorOr<std::reference_wrapper<const ImageData>> getImageData(const std::string& filePath);

  ErrorOr<std::reference_wrapper<const VertexData>> getVertexData(const std::string& filePath);

private:
  void loadImageAsync(
      const std::string& filePath,
      std::function<ErrorOr<ImageResource>(std::span<const std::byte>)>&& loadingFunction);

  std::launch _launchPolicy;

  const LogicalDevice* _logicalDevice = nullptr;

  std::shared_ptr<FileLoader> _fileLoader;

  std::unordered_map<std::string, VertexData> _vertexDataResources;
  std::unordered_map<std::string, std::future<ErrorOr<VertexData>>> _awaitingVertexDataResources;

  std::unordered_map<std::string, ImageData> _imageResources;
  std::unordered_map<std::string, std::future<ErrorOr<ImageData>>> _awaitingImageResources;
};

template <typename Model, typename... Type>
void AssetManager::loadVertexDataInterleavingAsync(
    std::shared_ptr<Model>& modelPtr, const std::string& name, std::span<const std::byte> indices,
    uint8_t indexSize, std::span<const std::pair<std::string, std::string>> orders,
    std::span<const Type>... attributes) {
  if (_awaitingVertexDataResources.contains(name)) {
    return;
  }
  
  auto future = std::async(
      _launchPolicy,
      [this, modelPtr, indices, indexSize, orders, attributes...]() -> ErrorOr<VertexData> {  // TODO: boost::asio::post,
                                            // boost::asio::use_future

        VertexData vertexData;
        
        AttributeDescription descs[] = {
          AttributeDescription{(void*)attributes.data(), sizeof(Type), attributes.size()}...
        };

        size_t stride = std::accumulate(std::cbegin(descs), std::cend(descs), 0,
                                        [](size_t val, const AttributeDescription& attribute) {
                                          return val + attribute.size;
                                        });

        for (const std::pair<std::string, std::string>& order : orders) {
          lib::Buffer<AttributeDescription> orderedDescs(order.second.size());
          size_t size = 0;
          for (size_t i = 0; i < orderedDescs.size(); i++) {
            orderedDescs[i] = descs[static_cast<size_t>(order.second[i] - '0')];
            size += orderedDescs[i].size * orderedDescs[i].count;
          }

          ASSIGN_OR_RETURN(
            auto vertexBuffer,
            Buffer::createStagingBuffer(*_logicalDevice, size));
          RETURN_IF_ERROR(vertexBuffer.copyDataInterleaving(
              orderedDescs));
          vertexData.buffers.emplace(order.first, std::move(vertexBuffer));
        }

        ASSIGN_OR_RETURN(vertexData.indexBuffer, Buffer::createStagingBuffer(*_logicalDevice, indices.size()));
        RETURN_IF_ERROR(vertexData.indexBuffer.copyData(indices));
        vertexData.indexType = getIndexType(indexSize);

        return vertexData;
      });
  _awaitingVertexDataResources.emplace(name, std::move(future));
}

template <typename Type, typename Model>
void AssetManager::loadVertexDataAsync(
    std::shared_ptr<Model>& modelPtr, const std::string& name, std::span<const std::byte> indices,
    uint8_t indexSize, std::span<const Type> vertices) {
  if (_awaitingVertexDataResources.contains(name)) {
    return;
  }
  auto future = std::async(
      _launchPolicy, ([this, modelPtr, indices, indexSize,
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
