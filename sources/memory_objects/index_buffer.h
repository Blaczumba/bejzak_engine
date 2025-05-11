#pragma once

#include "lib/status/status.h"
#include "memory_objects/staging_buffer.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <vector>

class CommandPool;
class LogicalDevice;

class IndexBuffer {
    VkBuffer _indexBuffer;
    Allocation _allocation;
    VkIndexType _indexType;
    uint32_t _indexCount;

    const LogicalDevice& _logicalDevice;

    IndexBuffer(const VkBuffer indexBuffer, const Allocation allocation, const LogicalDevice& logicalDevice, VkIndexType indexType, uint32_t indexCount);

public:
    ~IndexBuffer();

    static lib::ErrorOr<std::unique_ptr<IndexBuffer>> create(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, VkIndexType indexType);

    VkIndexType getIndexType() const;
    const VkBuffer getVkBuffer() const;
    uint32_t getIndexCount() const;
    void bind(const VkCommandBuffer commandBuffer) const;

private:
    struct Allocator {
        const size_t size;
        lib::ErrorOr<std::pair<VkBuffer, Allocation>> operator()(VmaWrapper& allocator) {
            ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE));
            return std::pair{ buffer.buffer, buffer.allocation };
        }

        lib::ErrorOr<std::pair<VkBuffer, Allocation>> operator()(auto&&) {
            return lib::Error("Unrecognized allocator in IndexBuffer creation");
        }
    };
};
