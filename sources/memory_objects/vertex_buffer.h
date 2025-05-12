#pragma once

#include "buffers.h"
#include "command_buffer/command_buffer.h"
#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "memory_objects/staging_buffer.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <vector>

class VertexBuffer {
    VkBuffer _vertexBuffer;
    Allocation _allocation;

	LogicalDevice* _logicalDevice;

	VertexBuffer(const VkBuffer vertexBuffer, const Allocation allocation, LogicalDevice& logicalDevice);

public:
    VertexBuffer();

    VertexBuffer(VertexBuffer&& vertexBuffer) noexcept;

    VertexBuffer& operator=(VertexBuffer&& vertexBuffer) noexcept;

    ~VertexBuffer();

    static lib::ErrorOr<VertexBuffer> create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer);

    static lib::ErrorOr<std::unique_ptr<VertexBuffer>> createPtr(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer);

    const VkBuffer getVkBuffer() const;

    void bind(const VkCommandBuffer commandBuffer) const;

private:
    struct Allocator {
        const size_t size;

        lib::ErrorOr<std::pair<VkBuffer, Allocation>> operator()(VmaWrapper& allocator) {
            ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
            return std::make_pair(buffer.buffer, buffer.allocation);
        }

        lib::ErrorOr<std::pair<VkBuffer, Allocation>> operator()(auto&&) {
            return lib::Error("Unrecognized allocator in VertexBuffer creation");
        }
    };
};
