#include "vertex_buffer.h"

VertexBuffer::VertexBuffer() : _vertexBuffer(VK_NULL_HANDLE) {};

VertexBuffer::VertexBuffer(const VkBuffer vertexBuffer, const Allocation allocation, LogicalDevice& logicalDevice)
    : _vertexBuffer(vertexBuffer), _allocation(allocation), _logicalDevice(&logicalDevice) {}

VertexBuffer::VertexBuffer(VertexBuffer&& vertexBuffer) noexcept
    : _vertexBuffer(std::exchange(vertexBuffer._vertexBuffer, VK_NULL_HANDLE)), _allocation(vertexBuffer._allocation), _logicalDevice(vertexBuffer._logicalDevice) {

}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& vertexBuffer) noexcept {
    if (this == &vertexBuffer) {
        return *this;
    }
    _vertexBuffer = std::exchange(vertexBuffer._vertexBuffer, VK_NULL_HANDLE);
    _allocation = vertexBuffer._allocation;
    _logicalDevice = vertexBuffer._logicalDevice;
}

VertexBuffer::~VertexBuffer() {
    if (_vertexBuffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _vertexBuffer }, _logicalDevice->getMemoryAllocator(), _allocation);
    }
}

lib::ErrorOr<VertexBuffer> VertexBuffer::create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer) {
    ASSIGN_OR_RETURN(const auto bufferInfo, std::visit(Allocator{ stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), bufferInfo.first, stagingBuffer.getSize());
    return VertexBuffer(bufferInfo.first, bufferInfo.second, logicalDevice);
}

lib::ErrorOr<std::unique_ptr<VertexBuffer>> VertexBuffer::createPtr(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer) {
    ASSIGN_OR_RETURN(const auto bufferInfo, std::visit(Allocator{ stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), bufferInfo.first, stagingBuffer.getSize());
    return std::unique_ptr<VertexBuffer>(new VertexBuffer(bufferInfo.first, bufferInfo.second, logicalDevice));
}

const VkBuffer VertexBuffer::getVkBuffer() const {
    return _vertexBuffer;
}

void VertexBuffer::bind(const VkCommandBuffer commandBuffer) const {
    const VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer, offsets);
}
