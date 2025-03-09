#include "vertex_buffer.h"

VertexBuffer::VertexBuffer(const VkBuffer vertexBuffer, const Allocation allocation, const LogicalDevice& logicalDevice)
    : _vertexBuffer(vertexBuffer), _allocation(allocation), _logicalDevice(logicalDevice) {}

VertexBuffer::~VertexBuffer() {
    std::visit(BufferDeallocator{ _vertexBuffer }, _logicalDevice.getMemoryAllocator(), _allocation);
}

lib::ErrorOr<std::unique_ptr<VertexBuffer>> VertexBuffer::create(const LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer) {
    Allocation allocation;
    const VkBuffer vertexBuffer = std::visit(Allocator{ allocation, stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator());
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), vertexBuffer, stagingBuffer.getSize());
    return std::unique_ptr<VertexBuffer>(new VertexBuffer(vertexBuffer, allocation, logicalDevice));
}

const VkBuffer VertexBuffer::getVkBuffer() const {
    return _vertexBuffer;
}

void VertexBuffer::bind(const VkCommandBuffer commandBuffer) const {
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer, offsets);
}
