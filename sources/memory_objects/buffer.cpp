#include "buffer.h"

#include <iostream>

Buffer::Buffer() : _usage(0), _buffer(VK_NULL_HANDLE), _size(0), _mappedMemory(nullptr) {};

Buffer::Buffer(LogicalDevice& logicalDevice, const Allocation allocation, const VkBuffer buffer, VkBufferUsageFlags usage, uint32_t size, void* mappedData)
    : _logicalDevice(&logicalDevice), _allocation(allocation), _buffer(buffer), _usage(usage), _size(size), _mappedMemory(mappedData) {
}

Buffer::Buffer(Buffer&& buffer) noexcept
    : _buffer(std::exchange(buffer._buffer, VK_NULL_HANDLE)), _allocation(buffer._allocation), _logicalDevice(buffer._logicalDevice),
    _usage(buffer._usage), _size(buffer._size), _mappedMemory(std::exchange(buffer._mappedMemory, nullptr)) {

}

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
        std::visit(BufferDeallocator{ _buffer }, _logicalDevice->getMemoryAllocator(), _allocation);
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

    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
        return BufferData{ buffer.buffer, buffer.allocation, usage };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator in VertexBuffer creation");
    }
};

struct IndexBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
        return BufferData{ buffer.buffer, buffer.allocation, usage };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator in IndexBuffer creation");
    }
};

struct StagingBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& wrapper) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, wrapper.createVkBuffer(size, usage, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
        return BufferData{ buffer.buffer, buffer.allocation, usage, buffer.mappedData };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator during StagingBuffer creation");
    }
};

struct UniformBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
        return BufferData{ buffer.buffer, buffer.allocation, usage, buffer.mappedData };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Not recognized allocator for UniformBufferData creation");
    }
};

} // namespace

lib::ErrorOr<Buffer> Buffer::createVertexBuffer(LogicalDevice& logicalDevice, uint32_t size) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(VertexBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size, bufferData.mappedMemory);
}

lib::ErrorOr<Buffer> Buffer::createIndexBuffer(LogicalDevice& logicalDevice, uint32_t size) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(IndexBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size, bufferData.mappedMemory);
}

lib::ErrorOr<Buffer> Buffer::createStagingBuffer(LogicalDevice& logicalDevice, uint32_t size) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(StagingBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size, bufferData.mappedMemory);
}

lib::ErrorOr<Buffer> Buffer::createUniformBuffer(LogicalDevice& logicalDevice, uint32_t size) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(UniformBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(logicalDevice, bufferData.allocation, bufferData.buffer, bufferData.usage, size, bufferData.mappedMemory);
}

lib::Status Buffer::copyBuffer(const VkCommandBuffer commandBuffer, const Buffer& srcBuffer, std::optional<VkDeviceSize> srcSize, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    if ((_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) == 0) [[unlikely]] {
        return lib::Error("Buffer does not have VK_BUFFER_USAGE_TRANSFER_DST_BIT specified.");
    }
    if ((srcBuffer._usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) [[unlikely]] {
        return lib::Error("Source Buffer does not have VK_BUFFER_USAGE_TRANSFER_SRC_BIT specified.");
    }
    const VkDeviceSize size = srcSize.value_or(srcBuffer._size);
    if (srcOffset + size > srcBuffer._size) [[unlikely]] {
        return lib::Error("Out of bounds while copying from srcBuffer.");
    }
    if (dstOffset + size > _size) [[unlikely]] {
        return lib::Error("Out of bounds while copying to the buffer.");
    }
    copyBufferToBuffer(commandBuffer, srcBuffer._buffer, _buffer, srcOffset, dstOffset, size);
    return lib::StatusOk();
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
