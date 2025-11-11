#pragma once

#include <array>
#include <cstring>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "buffer_deallocator.h"
#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/logical_device/logical_device.h"

struct AttributeDescription {
  void* data;
  size_t size;
  size_t count;
};

class Buffer {
public:
  Buffer() = default;

  Buffer(Buffer&& Buffer) noexcept;

  Buffer& operator=(Buffer&& Buffer) noexcept;

  ~Buffer();

  static ErrorOr<Buffer> createVertexBuffer(const LogicalDevice& logicalDevice, uint32_t size);

  static ErrorOr<Buffer> createIndexBuffer(const LogicalDevice& logicalDevice, uint32_t size);

  static ErrorOr<Buffer> createStagingBuffer(const LogicalDevice& logicalDevice, uint32_t size);

  static ErrorOr<Buffer> createUniformBuffer(const LogicalDevice& logicalDevice, uint32_t size);

  Status copyBuffer(const VkCommandBuffer commandBuffer, const Buffer& srcBuffer,
                    std::optional<VkDeviceSize> size = std::nullopt, VkDeviceSize srcOffset = 0,
                    VkDeviceSize dstOffset = 0);

  Status copyDataInterleaving(
      std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords);

  Status copyDataInterleaving(
      std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
      std::span<const glm::vec3> normals);

  Status copyDataInterleaving(std::span<const AttributeDescription> attributes);

  template <typename T>
  requires std::is_unsigned_v<T>
  ErrorOr<size_t> copyAndShrinkData(std::span<const T> data, VkDeviceSize offset = 0);

  template <typename T>
  Status copyData(std::span<const T> data, VkDeviceSize offset = 0);

  template <typename T>
  Status copyData(const T& data, VkDeviceSize offset = 0);

  VkBufferUsageFlags getUsage() const;

  uint32_t getSize() const;

  void* getMappedMemory() const;

  const VkBuffer& getVkBuffer() const;

private:
  VkBuffer _buffer = VK_NULL_HANDLE;
  Allocation _allocation;
  VkDeviceSize _size;
  VkBufferUsageFlags _usage;
  void* _mappedMemory = nullptr;

  const LogicalDevice* _logicalDevice;

  Buffer(const LogicalDevice& logicalDevice, const Allocation allocation,
         const VkBuffer vertexBuffer, VkBufferUsageFlags usage, uint32_t size,
         void* mappedData = nullptr);
};

template <typename T>
requires std::is_unsigned_v<T>
ErrorOr<size_t> Buffer::copyAndShrinkData(std::span<const T> data, VkDeviceSize offset) {
  if (!_mappedMemory) {
    return Error(EngineError::NOT_MAPPED);
  }

  const size_t maxIndex = *std::max_element(std::cbegin(data), std::cend(data));
  void* mappedMemory = static_cast<uint8_t*>(_mappedMemory) + offset;
  if (maxIndex <= std::numeric_limits<uint8_t>::max()) {
    if (_size < (maxIndex + 1) * sizeof(uint8_t) + offset) {
      return Error(EngineError::INDEX_OUT_OF_RANGE);
    }
    for (size_t i = 0; i < data.size(); i++) {
      static_cast<uint8_t*>(mappedMemory)[i] = static_cast<uint8_t>(data[i]);
    }
    return sizeof(uint8_t);
  } else if (maxIndex <= std::numeric_limits<uint16_t>::max()) {
    if (_size < (maxIndex + 1) * sizeof(uint16_t) + offset) {
      return Error(EngineError::INDEX_OUT_OF_RANGE);
    }
    for (size_t i = 0; i < data.size(); i++) {
      static_cast<uint16_t*>(mappedMemory)[i] = static_cast<uint16_t>(data[i]);
    }
    return sizeof(uint16_t);
  } else {
    if (_size < (maxIndex + 1) * sizeof(uint32_t) + offset) {
      return Error(EngineError::INDEX_OUT_OF_RANGE);
    }
    std::memcpy(static_cast<uint8_t*>(mappedMemory), data.data(), data.size() * sizeof(uint32_t));
    return sizeof(uint32_t);
  }
}

template <typename T>
Status Buffer::copyData(std::span<const T> data, VkDeviceSize offset) {
  if (!_mappedMemory) [[unlikely]] {
    return Error(EngineError::NOT_MAPPED);
  }
  const uint32_t size = data.size() * sizeof(T);
  if (offset + size > _size) [[unlikely]] {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  std::memcpy(static_cast<uint8_t*>(_mappedMemory) + offset, data.data(), size);
  return StatusOk();
}

template <typename T>
Status Buffer::copyData(const T& data, VkDeviceSize offset) {
  if (!_mappedMemory) [[unlikely]] {
    return Error(EngineError::NOT_MAPPED);
  }
  if (offset + sizeof(T) > _size) [[unlikely]] {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  std::memcpy(static_cast<uint8_t*>(_mappedMemory) + offset, &data, sizeof(T));
  return StatusOk();
}
