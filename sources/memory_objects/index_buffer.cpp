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

IndexBuffer::IndexBuffer(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, VkIndexType indexType)
    : _logicalDevice(logicalDevice), _indexCount(stagingBuffer.getSize() / getIndexSize(indexType)), _indexType(indexType) {
    _indexBuffer = std::visit(Allocator{ _allocation, stagingBuffer.getSize() }, _logicalDevice.getMemoryAllocator());
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), _indexBuffer, stagingBuffer.getSize());
}

IndexBuffer::~IndexBuffer() {
    if (_indexBuffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _indexBuffer }, _logicalDevice.getMemoryAllocator(), _allocation);
    }
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
