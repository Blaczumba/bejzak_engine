#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "memory_objects/staging_buffer.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <vector>

class Buffer {
public:
    Buffer();

    Buffer(Buffer&& vertexBuffer) noexcept;

    Buffer& operator=(Buffer&& vertexBuffer) noexcept;

    ~Buffer();

    enum class Type : uint8_t {
        NONE,
        VERTEX,
        INDEX,
        STAGING,
        UNIFORM
    };

    // Creates Buffer of any type with given size.
    template<typename Allocator>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, uint32_t size);

    // Creates Buffer and copies data from stagingBuffer.
    template<typename Allocator>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const Buffer& stagingBuffer);

    // Creates Buffer (staging buffer) and copies data to it.
    template<BufferLike Data>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, const Data& data);

    Type getType() const;

    uint32_t getSize() const;

    void* getMappedMemory() const;

private:
    Type _type;

    VkBuffer _buffer;
    Allocation _allocation;
    uint32_t _size;
    void* _mappedMemory;

    LogicalDevice* _logicalDevice;

    Buffer(Type type, const VkBuffer vertexBuffer, const Allocation allocation, LogicalDevice& logicalDevice, uint32_t size, void* mappedData = nullptr);
};

struct BufferData {
    Buffer::Type type;
    VkBuffer buffer;
    Allocation allocation;
    void* mappedMemory;
};

struct VertexBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
        return BufferData{ Buffer::Type::VERTEX, buffer.buffer, buffer.allocation };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator in VertexBuffer creation");
    }
};

struct IndexBufferAllocator {
    const size_t size;
    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
        return BufferData{ Buffer::Type::INDEX, buffer.buffer, buffer.allocation };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator in IndexBuffer creation");
    }
};

struct StagingBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& wrapper) {
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, wrapper.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
        return BufferData{ Buffer::Type::STAGING, buffer.buffer, buffer.allocation, buffer.mappedData };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Unrecognized allocator during StagingBuffer creation");
    }
};

struct UniformBufferAllocator {
    const size_t size;

    lib::ErrorOr<BufferData> operator()(VmaWrapper& allocator) {
        ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
        return BufferData{ Buffer::Type::STAGING, buffer.buffer, buffer.allocation, buffer.mappedData };
    }

    lib::ErrorOr<BufferData> operator()(auto&&) {
        return lib::Error("Not recognized allocator for UniformBufferData creation");
    }
};

template<typename Allocator>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, uint32_t size) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(Allocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, size);
}

template<typename Allocator>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const Buffer& stagingBuffer) {
    if (stagingBuffer._type != Type::STAGING) {
        return lib::Error("Cannot copy from buffer!");
    }
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(Allocator{ stagingBuffer._size }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer._buffer, bufferData.buffer, stagingBuffer._size);
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, stagingBuffer._size);
}

template<BufferLike Data>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, const Data& data) {
    const uint32_t size = data.size() * sizeof(std::decay_t<decltype(*data.data())>);
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(StagingBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    std::memcpy(bufferData.data, data.data(), size);
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, size);
}
