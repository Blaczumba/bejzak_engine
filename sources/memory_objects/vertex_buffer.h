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

	const LogicalDevice& _logicalDevice;

	VertexBuffer(const VkBuffer vertexBuffer, const Allocation allocation, const LogicalDevice& logicalDevice);

public:
    ~VertexBuffer();

    static lib::ErrorOr<std::unique_ptr<VertexBuffer>> create(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer);

    const VkBuffer getVkBuffer() const;
    void bind(const VkCommandBuffer commandBuffer) const;

private:
    struct Allocator {
        Allocation& allocation;
        const size_t size;

        lib::ErrorOr<VkBuffer> operator()(VmaWrapper& allocator) {
            ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
            allocation = buffer.allocation;
            return buffer.buffer;
        }

        lib::ErrorOr<VkBuffer> operator()(auto&&) {
            return lib::Error("Unrecognized allocator in VertexBuffer creation");
        }
    };
};
