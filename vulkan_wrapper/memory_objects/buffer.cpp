#include "buffer.h"

#include <glm/glm.hpp>

#include "common/util/vertex_builder.h"
#include "vulkan_wrapper/memory_objects/buffers.h"

Buffer::Buffer(const LogicalDevice& logicalDevice, const Allocation allocation, const VkBuffer buffer,
               VkBufferUsageFlags usage, uint32_t size, void* mappedData)
  : _logicalDevice(&logicalDevice), _allocation(allocation), _buffer(buffer), _usage(usage),
    _size(size), _mappedMemory(mappedData) {}

Buffer::Buffer(Buffer&& buffer) noexcept
  : _buffer(std::exchange(buffer._buffer, VK_NULL_HANDLE)), _allocation(buffer._allocation),
    _logicalDevice(buffer._logicalDevice), _usage(buffer._usage), _size(buffer._size),
    _mappedMemory(std::exchange(buffer._mappedMemory, nullptr)) {}

Buffer& Buffer::operator=(Buffer&& buffer) noexcept {
  if (this == &buffer) {
    return *this;
  }
  // TODO what if _vertexBuffer != VK_NULL_HANDLE
  _buffer = std::exchange(buffer._buffer, VK_NULL_HANDLE);
  _allocation = buffer._allocation;
  _size = buffer._size;
  _usage = buffer._usage;
  _mappedMemory = std::exchange(buffer._mappedMemory, nullptr);
  _logicalDevice = buffer._logicalDevice;
  return *this;
}

Buffer::~Buffer() {
  if (_buffer != VK_NULL_HANDLE) {
    std::visit(BufferDeallocator{_buffer}, _logicalDevice->getMemoryAllocator(), _allocation);
  }
}

namespace {

struct BufferData {
  VkBuffer buffer;
  Allocation allocation;
  VkBufferUsageFlags usage;
  void* mappedMemory;
};

struct VertexBufferAllocator {
  const size_t size;

  ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer,
                     allocator.createVkBuffer(size, usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
    return BufferData{buffer.buffer, buffer.allocation, usage};
  }

  ErrorOr<BufferData> operator()(auto&&) {
    return Error(EngineError::NOT_RECOGNIZED_TYPE);
  }
};

struct IndexBufferAllocator {
  const size_t size;

  ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer,
                     allocator.createVkBuffer(size, usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
    return BufferData{buffer.buffer, buffer.allocation, usage};
  }

  ErrorOr<BufferData> operator()(auto&&) {
    return Error(EngineError::NOT_RECOGNIZED_TYPE);
  }
};

struct StagingBufferAllocator {
  const size_t size;

  ErrorOr<BufferData> operator()(VmaWrapper& wrapper) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer,
                     wrapper.createVkBuffer(size, usage, VMA_MEMORY_USAGE_CPU_ONLY,
                                            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                                                | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    return BufferData{buffer.buffer, buffer.allocation, usage, buffer.mappedData};
  }

  ErrorOr<BufferData> operator()(auto&&) {
    return Error(EngineError::NOT_RECOGNIZED_TYPE);
  }
};

struct UniformBufferAllocator {
  const size_t size;

  ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer,
                     allocator.createVkBuffer(size, usage, VMA_MEMORY_USAGE_CPU_ONLY,
                                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                                                  | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    return BufferData{buffer.buffer, buffer.allocation, usage, buffer.mappedData};
  }

  ErrorOr<BufferData> operator()(auto&&) {
    return Error(EngineError::NOT_RECOGNIZED_TYPE);
  }
};

}  // namespace

ErrorOr<Buffer> Buffer::createVertexBuffer(const LogicalDevice& logicalDevice, uint32_t size) {
  ASSIGN_OR_RETURN(const BufferData bufferData,
                   std::visit(VertexBufferAllocator{size}, logicalDevice.getMemoryAllocator()));
  return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size,
                bufferData.mappedMemory);
}

ErrorOr<Buffer> Buffer::createIndexBuffer(const LogicalDevice& logicalDevice, uint32_t size) {
  ASSIGN_OR_RETURN(const BufferData bufferData,
                   std::visit(IndexBufferAllocator{size}, logicalDevice.getMemoryAllocator()));
  return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size,
                bufferData.mappedMemory);
}

ErrorOr<Buffer> Buffer::createStagingBuffer(const LogicalDevice& logicalDevice, uint32_t size) {
  ASSIGN_OR_RETURN(const BufferData bufferData,
                   std::visit(StagingBufferAllocator{size}, logicalDevice.getMemoryAllocator()));
  return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size,
                bufferData.mappedMemory);
}

ErrorOr<Buffer> Buffer::createUniformBuffer(const LogicalDevice& logicalDevice, uint32_t size) {
  ASSIGN_OR_RETURN(const BufferData bufferData,
                   std::visit(UniformBufferAllocator{size}, logicalDevice.getMemoryAllocator()));
  return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size,
                bufferData.mappedMemory);
}

Status Buffer::copyBuffer(
    const VkCommandBuffer commandBuffer, const Buffer& srcBuffer,
    std::optional<VkDeviceSize> srcSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
  if ((_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }
  if ((srcBuffer._usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }
  const VkDeviceSize size = srcSize.value_or(srcBuffer._size);
  if (srcOffset + size > srcBuffer._size) [[unlikely]] {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  if (dstOffset + size > _size) [[unlikely]] {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  copyBufferToBuffer(commandBuffer, srcBuffer._buffer, _buffer, srcOffset, dstOffset, size);
  return StatusOk();
}

Status Buffer::copyDataInterleaving(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords) {
  if (!_mappedMemory) [[unlikely]] {
    return Error(EngineError::NOT_MAPPED);
  }
  return buildInterleavingVertexData(
      std::span(static_cast<std::byte*>(_mappedMemory), _size), positions, texCoords);
}

Status Buffer::copyDataInterleaving(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals) {
  if (!_mappedMemory) [[unlikely]] {
    return Error(EngineError::NOT_MAPPED);
  }
  return buildInterleavingVertexData(
      std::span(static_cast<std::byte*>(_mappedMemory), _size), positions, texCoords, normals);
}

Status Buffer::copyDataInterleaving(
    std::span<const glm::vec3> positions, std::span<const glm::vec2> texCoords,
    std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents) {
  if (!_mappedMemory) [[unlikely]] {
    return Error(EngineError::NOT_MAPPED);
  }
  return buildInterleavingVertexData(std::span(static_cast<std::byte*>(_mappedMemory), _size),
                                     positions, texCoords, normals, tangents);
}

VkBufferUsageFlags Buffer::getUsage() const {
  return _usage;
}

uint32_t Buffer::getSize() const {
  return _size;
}

void* Buffer::getMappedMemory() const {
  return _mappedMemory;
}

const VkBuffer& Buffer::getVkBuffer() const {
  return _buffer;
}
