#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "buffer_deallocator.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <vector>

class Buffer {
public:
    Buffer();

    Buffer(Buffer&& Buffer) noexcept;

    Buffer& operator=(Buffer&& Buffer) noexcept;

    ~Buffer();

    static lib::ErrorOr<Buffer> createVertexBuffer(LogicalDevice& logicalDevice, uint32_t size);

    static lib::ErrorOr<Buffer> createIndexBuffer(LogicalDevice& logicalDevice, uint32_t size);

    static lib::ErrorOr<Buffer> createStagingBuffer(LogicalDevice& logicalDevice, uint32_t size);

    static lib::ErrorOr<Buffer> createUniformBuffer(LogicalDevice& logicalDevice, uint32_t size);

    lib::Status copyBuffer(const VkCommandBuffer commandBuffer, const Buffer& srcBuffer, std::optional<VkDeviceSize> size = std::nullopt, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

    template<typename T>
    lib::Status copyData(std::span<const T> data);

    VkBufferUsageFlags getUsage() const;

    uint32_t getSize() const;

    void* getMappedMemory() const;

    const VkBuffer& getVkBuffer() const;

private:
    VkBuffer _buffer;
    Allocation _allocation;
    VkDeviceSize _size;
    VkBufferUsageFlags _usage;
    void* _mappedMemory;

    LogicalDevice* _logicalDevice;

    Buffer(LogicalDevice& logicalDevice, const Allocation allocation, const VkBuffer vertexBuffer, VkBufferUsageFlags usage, uint32_t size, void* mappedData = nullptr);
};

struct BufferData {
    VkBuffer buffer;
    Allocation allocation;
    VkBufferUsageFlags usage;
    void* mappedMemory;
};

namespace {

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

template<typename T>
lib::Status Buffer::copyData(std::span<const T> data) {
    if (!_mappedMemory) [[unlikely]] {
        return lib::Error("The memory is not mapped!");
    }
    const uint32_t size = data.size() * sizeof(T);
    if (size > _size) [[unlikely]] {
        return lib::Error("Out of bounds while copying data to buffer!");
    }
    std::memcpy(_mappedMemory, data.data(), size);
    return lib::StatusOk();
}
