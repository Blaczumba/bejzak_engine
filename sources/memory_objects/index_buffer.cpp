#include "index_buffer.h"

#include "buffers.h"
#include "command_buffer/command_buffer.h"
#include "logical_device/logical_device.h"

namespace {

constexpr uint8_t getIndexSize(VkIndexType indexType) {
    switch (indexType) {
    case VK_INDEX_TYPE_UINT8_EXT:
        return 1;
    case VK_INDEX_TYPE_UINT16:
        return 2;
    case VK_INDEX_TYPE_UINT32:
        return 4;
    default:
        return 0;
    }
}

}

IndexBuffer::IndexBuffer(const VkBuffer indexBuffer, const Allocation allocation, const LogicalDevice& logicalDevice, VkIndexType indexType, uint32_t indexCount)
    : _indexBuffer(indexBuffer), _allocation(allocation), _logicalDevice(logicalDevice), _indexType(indexType), _indexCount(indexCount) {

}

lib::ErrorOr<std::unique_ptr<IndexBuffer>> IndexBuffer::create(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, VkIndexType indexType) {
    Allocation allocation;
    ASSIGN_OR_RETURN(VkBuffer indexBuffer, std::visit(Allocator{ allocation, stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), indexBuffer, stagingBuffer.getSize());
    return std::unique_ptr<IndexBuffer>(new IndexBuffer(indexBuffer, allocation, logicalDevice, indexType, stagingBuffer.getSize() / getIndexSize(indexType)));
}

IndexBuffer::~IndexBuffer() {
    std::visit(BufferDeallocator{ _indexBuffer }, _logicalDevice.getMemoryAllocator(), _allocation);
}

const VkBuffer IndexBuffer::getVkBuffer() const {
    return _indexBuffer;
}

VkIndexType IndexBuffer::getIndexType() const {
    return _indexType;
}

uint32_t IndexBuffer::getIndexCount() const {
    return _indexCount;
}

void IndexBuffer::bind(const VkCommandBuffer commandBuffer) const {
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, _indexType);
}
