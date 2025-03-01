#include "vertex_buffer.h"

VertexBuffer::VertexBuffer(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer)
    : _logicalDevice(logicalDevice) {
    _vertexBuffer = std::visit(Allocator{ _allocation, stagingBuffer.getSize() }, _logicalDevice.getMemoryAllocator());
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), _vertexBuffer, stagingBuffer.getSize());
}

VertexBuffer::~VertexBuffer() {
    if (_vertexBuffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _vertexBuffer }, _logicalDevice.getMemoryAllocator(), _allocation);
    }
}

const VkBuffer VertexBuffer::getVkBuffer() const {
    return _vertexBuffer;
}

void VertexBuffer::bind(const VkCommandBuffer commandBuffer) const {
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer, offsets);
}
