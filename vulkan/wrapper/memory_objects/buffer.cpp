#include "buffer.h"

#include <optional>
#include <span>
#include <tuple>
#include <variant>
#include <vulkan/vulkan.h>

std::span<const std::byte> BufferMetadata::getMappedMemoryAsSpan() const noexcept {
  return std::span(mappedMemory, size);
}

std::span<std::byte> BufferMetadata::getMappedMemoryAsSpan() noexcept {
  return std::span(mappedMemory, size);
}

Buffer::Buffer(const LogicalDevice& logicalDevice, VkBuffer buffer, Allocation allocation) noexcept
  : _logicalDevice(&logicalDevice), _buffer(buffer), _allocation(allocation) {}

Buffer::Buffer(Buffer&& buffer) noexcept
  : _buffer(std::exchange(buffer._buffer, VK_NULL_HANDLE)), _allocation(buffer._allocation),
    _logicalDevice(std::exchange(buffer._logicalDevice, nullptr)) {}

Buffer& Buffer::operator=(Buffer&& buffer) noexcept {
  if (this == &buffer) {
    return *this;
  }

  if (_buffer != VK_NULL_HANDLE) {
    destroy();
  }

  _buffer = std::exchange(buffer._buffer, VK_NULL_HANDLE);
  _allocation = buffer._allocation;
  _logicalDevice = std::exchange(buffer._logicalDevice, nullptr);
  return *this;
}

namespace {

struct BufferDeallocator {
  const VkBuffer buffer;

  void operator()(VmaWrapper& allocator, const VmaAllocation allocation) {
    allocator.destroyVkBuffer(buffer, allocation);
  }

  void operator()(auto&&, auto&&) {}
};

}  // namespace

void Buffer::destroy() {
  _logicalDevice->destroyResource(
      [buffer = _buffer, allocation = _allocation](DestroyerContext context) {
        std::visit(BufferDeallocator{buffer}, *context.memoryAllocator, allocation);
      });
}

Buffer::~Buffer() {
  if (_buffer != VK_NULL_HANDLE) {
    destroy();
    // TODO: Remove it once we enhance the collections of data.
    _buffer = VK_NULL_HANDLE;
  }
}

namespace {

struct BufferResources {
  VkBuffer buffer;
  Allocation allocation;
  void* mappedMemory;
};

struct VertexInputBufferAllocator {
  const VkBufferCreateInfo& bufferCreateInfo;

  BufferResources operator()(VmaWrapper& allocator) {
    const VmaWrapper::Buffer buffer =
        allocator.createVkBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    return BufferResources{buffer.buffer, buffer.allocation};
  }

  BufferResources operator()(auto&&) {
    return {};
  }
};

struct StagingBufferAllocator {
  const VkBufferCreateInfo& bufferCreateInfo;

  BufferResources operator()(VmaWrapper& wrapper) {
    const VmaWrapper::Buffer buffer = wrapper.createVkBuffer(
        bufferCreateInfo, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    return BufferResources{buffer.buffer, buffer.allocation, buffer.mappedData};
  }

  BufferResources operator()(auto&&) {
    return {};
  }
};

struct UniformBufferAllocator {
  const VkBufferCreateInfo& bufferCreateInfo;

  BufferResources operator()(VmaWrapper& allocator) {
    const VmaWrapper::Buffer buffer = allocator.createVkBuffer(
        bufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    return BufferResources{buffer.buffer, buffer.allocation, buffer.mappedData};
  }

  BufferResources operator()(auto&&) {
    return {};
  }
};

template <typename Allocator>
BufferResources createBuffer(
    const LogicalDevice& logicalDevice, const VkBufferCreateInfo& createInfo) {
  return std::visit(Allocator{createInfo}, logicalDevice.getMemoryAllocator());
}

}  // namespace

const VkBuffer& Buffer::getVkBuffer() const noexcept {
  return _buffer;
}

VkBuffer Buffer::getVkResource() const noexcept {
  return _buffer;
}

const LogicalDevice& Buffer::getLogicalDevice() const noexcept {
  return *_logicalDevice;
}

BufferBuilder&& BufferBuilder::withUsage(VkBufferUsageFlags usage) && noexcept {
  _createInfo.usage = usage;
  return std::move(*this);
}

BufferBuilder&& BufferBuilder::withSize(VkDeviceSize size) && noexcept {
  _createInfo.size = size;
  return std::move(*this);
}

BufferBuilder&& BufferBuilder::withQueueFamilyIndices(
    std::span<const uint32_t> queueFamilyIndices) && noexcept {
  _queueFamilyIndices.assign(std::cbegin(queueFamilyIndices), std::cend(queueFamilyIndices));
  _createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
  _createInfo.queueFamilyIndexCount = static_cast<uint32_t>(_queueFamilyIndices.size());
  _createInfo.pQueueFamilyIndices = _queueFamilyIndices.data();
  return std::move(*this);
}

BufferBuilder&& BufferBuilder::withFlags(VkBufferCreateFlags flags) && noexcept {
  _createInfo.flags = flags;
  return std::move(*this);
}

BufferMetadata BufferBuilder::buildMetadata() const noexcept {
  return BufferMetadata{
    .usage = _createInfo.usage,
    .size = _createInfo.size,
    .mappedMemory = _mappedMemory,
    .flags = _createInfo.flags,
    .sharingMode = _createInfo.sharingMode,
    .queueFamilyIndices = _queueFamilyIndices,
  };
}

Buffer BufferBuilder::buildVertexInputBuffer(const LogicalDevice& logicalDevice) {
  const BufferResources bufferResources =
      createBuffer<VertexInputBufferAllocator>(logicalDevice, _createInfo);
  _mappedMemory = reinterpret_cast<std::byte*>(bufferResources.mappedMemory);
  return Buffer(logicalDevice, bufferResources.buffer, bufferResources.allocation);
}

Buffer BufferBuilder::buildStagingBuffer(const LogicalDevice& logicalDevice) {
  const BufferResources bufferResources =
      createBuffer<StagingBufferAllocator>(logicalDevice, _createInfo);
  _mappedMemory = reinterpret_cast<std::byte*>(bufferResources.mappedMemory);
  return Buffer(logicalDevice, bufferResources.buffer, bufferResources.allocation);
}

Buffer BufferBuilder::buildUniformBuffer(const LogicalDevice& logicalDevice) {
  const BufferResources bufferResources =
      createBuffer<UniformBufferAllocator>(logicalDevice, _createInfo);
  _mappedMemory = reinterpret_cast<std::byte*>(bufferResources.mappedMemory);
  return Buffer(logicalDevice, bufferResources.buffer, bufferResources.allocation);
}

std::tuple<Buffer, BufferMetadata> BufferBuilder::buildVertexInputBufferWithMetadata(
    const LogicalDevice& logicalDevice) {
  // Do not inline to guarantee the order of execution.
  Buffer buffer = buildVertexInputBuffer(logicalDevice);
  return std::make_tuple(std::move(buffer), buildMetadata());
}

std::tuple<Buffer, BufferMetadata> BufferBuilder::buildStagingBufferWithMetadata(
    const LogicalDevice& logicalDevice) {
  // Do not inline to guarantee the order of execution.
  Buffer buffer = buildStagingBuffer(logicalDevice);
  return std::make_tuple(std::move(buffer), buildMetadata());
}

std::tuple<Buffer, BufferMetadata> BufferBuilder::buildUniformBufferWithMetadata(
    const LogicalDevice& logicalDevice) {
  // Do not inline to guarantee the order of execution.
  Buffer buffer = buildUniformBuffer(logicalDevice);
  return std::make_tuple(std::move(buffer), buildMetadata());
}
