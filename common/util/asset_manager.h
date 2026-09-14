#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <string>
#include <tuple>

#include "common/buffer/buffer_utils.h"
#include "common/buffer/index_buffer_utils.h"
#include "common/model_loader/image_loader/image_loader.h"
#include "common/model_loader/image_loader/types.h"
#include "common/util/ref.h"
#include "common/util/resource_handles.h"
#include "lib/association_list/association_list.h"
#include "lib/buffer/buffer.h"

namespace common {

class AssetManager {
public:
  enum class LoadState : uint8_t {
    PENDING,
    PARTIAL,
    FINISHED
  };

  struct ImageData {
    Ref<RefType::Buffer> stagingBuffer;
    Ref<RefType::VirtualAllocation> virtualAllocation;
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;
    lib::Buffer<ImageSubresource> copyRegions;
    std::atomic<uint8_t> residentMips = 0;
    std::atomic<LoadState> loadState = LoadState::PENDING;
  };

  struct VertexData {
    lib::DynamicAssociationList<std::string,
                                std::tuple<Ref<RefType::Buffer>, Ref<RefType::VirtualAllocation>>>
        buffers;
    std::tuple<Ref<RefType::Buffer>, Ref<RefType::VirtualAllocation>> indexBuffer;
    IndexType indexType;
    std::atomic<LoadState> loadState = LoadState::PENDING;
  };

  virtual std::shared_ptr<ImageData> loadImageAsync(
      std::function<std::tuple<ImageResource, OwnedImageData>(void)>&& imageFunction) = 0;

  virtual std::shared_ptr<ImageData> loadImageAsync(
      std::shared_ptr<void> modelPtr, ImageResource&& imageResource) = 0;

  virtual std::shared_ptr<VertexData> loadVertexDataInterleavingAsync(
      std::shared_ptr<void> modelPtr, std::span<const std::byte> indices, IndexType indexSize,
      std::vector<BufferDescription>&& bufferDescriptions) = 0;
};

}  // namespace common
