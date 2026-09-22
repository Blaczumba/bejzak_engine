#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class Buffer {
  Buffer(const LogicalDevice& logicalDevice, VkBuffer buffer, Allocation allocation) noexcept;

public:
  Buffer() noexcept = default;

  Buffer(Buffer&& buffer) noexcept;

  Buffer& operator=(Buffer&& buffer) noexcept;

  ~Buffer();

  // TODO: Do not return the reference.
  const VkBuffer& getVkBuffer() const noexcept;

  VkBuffer getVkResource() const noexcept;

  const LogicalDevice& getLogicalDevice() const noexcept;

private:
  void destroy();

  VkBuffer _buffer = VK_NULL_HANDLE;
  Allocation _allocation;

  const LogicalDevice* _logicalDevice;

  friend class BufferBuilder;
};

struct BufferMetadata {
  VkBufferUsageFlags usage;
  VkDeviceSize size;
  std::byte* mappedMemory;
  VkBufferCreateFlags flags;
  VkSharingMode sharingMode;
  std::vector<uint32_t> queueFamilyIndices;
  // Other std::optional fields representing pNext metadata.
  std::span<const std::byte> getMappedMemoryAsSpan() const noexcept;

  std::span<std::byte> getMappedMemoryAsSpan() noexcept;
};

class BufferBuilder {
public:
  BufferBuilder&& withUsage(VkBufferUsageFlags usage) && noexcept;

  BufferBuilder&& withSize(VkDeviceSize size) && noexcept;

  BufferBuilder&& withQueueFamilyIndices(std::span<const uint32_t> queueFamilyIndices) && noexcept;

  BufferBuilder&& withFlags(VkBufferCreateFlags flags) && noexcept;

  BufferMetadata buildMetadata() const noexcept;

  Buffer buildVertexInputBuffer(const LogicalDevice& logicalDevice);

  Buffer buildStagingBuffer(const LogicalDevice& logicalDevice);

  Buffer buildUniformBuffer(const LogicalDevice& logicalDevice);

  std::tuple<Buffer, BufferMetadata> buildVertexInputBufferWithMetadata(
      const LogicalDevice& logicalDevice);

  std::tuple<Buffer, BufferMetadata> buildStagingBufferWithMetadata(
      const LogicalDevice& logicalDevice);

  std::tuple<Buffer, BufferMetadata> buildUniformBufferWithMetadata(
      const LogicalDevice& logicalDevice);

private:
  VkBufferCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .size = 0,
    .usage = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = nullptr,
  };

  std::byte* _mappedMemory = nullptr;
  std::vector<uint32_t> _queueFamilyIndices;

  void* _pNext = nullptr;
};
