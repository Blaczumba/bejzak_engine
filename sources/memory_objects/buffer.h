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
    lib::Status copyData(std::span<const T> data, VkDeviceSize offset = 0);

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

template<typename T>
lib::Status Buffer::copyData(std::span<const T> data, VkDeviceSize offset) {
    if (!_mappedMemory) [[unlikely]] {
        return lib::Error("The memory is not mapped!");
    }
    const uint32_t size = data.size() * sizeof(T);
    if (offset + size > _size) [[unlikely]] {
        return lib::Error("Out of bounds while copying data to buffer!");
    }
    std::memcpy(static_cast<uint8_t*>(_mappedMemory) + offset, data.data(), size);
    return lib::StatusOk();
}
