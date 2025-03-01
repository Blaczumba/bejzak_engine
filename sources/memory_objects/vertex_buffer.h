#pragma once

#include "buffers.h"
#include "command_buffer/command_buffer.h"
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

public:
	VertexBuffer(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer);
    ~VertexBuffer();

    const VkBuffer getVkBuffer() const;
    void bind(const VkCommandBuffer commandBuffer) const;

private:
    struct Allocator {
        Allocation& allocation;
        const size_t size;

        const VkBuffer operator()(VmaWrapper& allocator) {
            auto [buffer, tmpAllocation, _] = allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
            allocation = tmpAllocation;
            return buffer;
        }

        const VkBuffer operator()(auto&&) {
            throw std::runtime_error("Unrecognized allocator in VertexBuffer creation");
        }
    };
};
