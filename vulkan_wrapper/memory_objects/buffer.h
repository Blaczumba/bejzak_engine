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

  Status copyDataInterleaving(
      std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
      std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents);

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
