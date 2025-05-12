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

IndexBuffer::IndexBuffer(const VkBuffer indexBuffer, const Allocation allocation, LogicalDevice& logicalDevice, VkIndexType indexType, uint32_t indexCount)
    : _indexBuffer(indexBuffer), _allocation(allocation), _logicalDevice(&logicalDevice), _indexType(indexType), _indexCount(indexCount) {

}

IndexBuffer::IndexBuffer() : _indexBuffer(VK_NULL_HANDLE) {

}

IndexBuffer::IndexBuffer(IndexBuffer&& indexBuffer) noexcept
    : _indexBuffer(std::exchange(indexBuffer._indexBuffer, VK_NULL_HANDLE)), _allocation(indexBuffer._allocation), _indexType(indexBuffer._indexType), _indexCount(indexBuffer._indexCount), _logicalDevice(indexBuffer._logicalDevice) {

}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& indexBuffer) noexcept {
    if (this == &indexBuffer) {
        return *this;
    }
    _indexBuffer = std::exchange(indexBuffer._indexBuffer, VK_NULL_HANDLE); 
    _allocation = indexBuffer._allocation;
    _indexType = indexBuffer._indexType;
    _indexCount = indexBuffer._indexCount;
    _logicalDevice = indexBuffer._logicalDevice;
}

IndexBuffer::~IndexBuffer() {
    if (_indexBuffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _indexBuffer }, _logicalDevice->getMemoryAllocator(), _allocation);
    }
}

lib::ErrorOr<IndexBuffer> IndexBuffer::create(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, VkIndexType indexType) {
    ASSIGN_OR_RETURN(const auto bufferInfo, std::visit(Allocator{ stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), bufferInfo.first, stagingBuffer.getSize());
    return IndexBuffer(bufferInfo.first, bufferInfo.second, logicalDevice, indexType, stagingBuffer.getSize() / getIndexSize(indexType));
}

lib::ErrorOr<std::unique_ptr<IndexBuffer>> IndexBuffer::createPtr(LogicalDevice& logicalDevice, const VkCommandBuffer commandBuffer, const StagingBuffer& stagingBuffer, VkIndexType indexType) {
    ASSIGN_OR_RETURN(const auto bufferInfo, std::visit(Allocator{ stagingBuffer.getSize() }, logicalDevice.getMemoryAllocator()));
    copyBufferToBuffer(commandBuffer, stagingBuffer.getVkBuffer(), bufferInfo.first, stagingBuffer.getSize());
    return std::unique_ptr<IndexBuffer>(new IndexBuffer(bufferInfo.first, bufferInfo.second, logicalDevice, indexType, stagingBuffer.getSize() / getIndexSize(indexType)));
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
