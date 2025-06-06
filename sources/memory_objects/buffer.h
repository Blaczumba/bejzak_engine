#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "buffer_deallocator.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <span>
#include <vector>

class Buffer {
public:
    Buffer();

    Buffer(Buffer&& vertexBuffer) noexcept;

    Buffer& operator=(Buffer&& vertexBuffer) noexcept;

    ~Buffer();

    // Creates Buffer of any type with given size.
    template<typename Allocator>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, uint32_t size, std::span<const std::pair<VkDeviceSize, uint32_t>> offsetStrides);

    // Creates Buffer and copies data from stagingBuffer.
    template<typename Allocator>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const Buffer& copyBuffer);

    // Creates Buffer (staging buffer) and copies data to it.
    template<typename T>
    static lib::ErrorOr<Buffer> create(LogicalDevice& logicalDevice, std::span<const T> data, std::span<const std::pair<VkDeviceSize, uint32_t>> offsetStrides = { {0, sizeof(T)} });

    VkBufferUsageFlags getUsage() const;

    uint32_t getSize() const;

    void* getMappedMemory() const;

    std::span<const std::pair<VkDeviceSize, uint32_t>> getOffsetStrides() const;

private:
    VkBuffer _buffer;
    Allocation _allocation;
    uint32_t _size;
    VkBufferUsageFlags _usage;
    std::vector<std::pair<VkDeviceSize, uint32_t>> _offsetStrides;
    void* _mappedMemory;

    LogicalDevice* _logicalDevice;

    Buffer(LogicalDevice& logicalDevice, const Allocation allocation, const VkBuffer vertexBuffer, VkBufferUsageFlags usage, uint32_t size, std::span<const std::pair<VkDeviceSize, uint32_t>> offsetStrides, void* mappedData = nullptr);
};

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
        return BufferData{  buffer.buffer, buffer.allocation, usage, buffer.mappedData };
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

template<typename Allocator>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, uint32_t size, std::span<const std::pair<VkDeviceSize, uint32_t>> offsetStrides) {
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(Allocator{ size }, logicalDevice.getMemoryAllocator()));
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, offsetStrides, size);
}

template<typename Allocator>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const Buffer& copyBuffer) {
    if ((copyBuffer._usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0) {
        return lib::Error("Copy buffer was not created with VK_BUFFER_USAGE_TRANSFER_SRC_BIT flag!");
    }
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(Allocator{ copyBuffer._size }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, copyBuffer._buffer, bufferData.buffer, copyBuffer._size);
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, copyBuffer._size, copyBuffer._offsetStrides);
}

template<typename T>
static lib::ErrorOr<Buffer> Buffer::create(LogicalDevice& logicalDevice, std::span<const T> data, std::span<const std::pair<VkDeviceSize, uint32_t>> offsetStrides) {
    const uint32_t size = data.size() * sizeof(T);
    ASSIGN_OR_RETURN(const BufferData bufferData, std::visit(StagingBufferAllocator{ size }, logicalDevice.getMemoryAllocator()));
    std::memcpy(bufferData.mappedMemory, data.data(), size);
    return Buffer(bufferData.type, bufferData.buffer, bufferData.allocation, logicalDevice, size, offsetStrides, bufferData.mappedMemory);
}
