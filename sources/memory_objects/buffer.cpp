#include "buffer.h"

Buffer::Buffer() : _usage(0), _buffer(VK_NULL_HANDLE), _size(0) {};

Buffer::Buffer(LogicalDevice& logicalDevice, const Allocation allocation, const VkBuffer buffer, VkBufferUsageFlags usage, uint32_t size, void* mappedData)
    : _logicalDevice(&logicalDevice), _allocation(allocation), _buffer(buffer), _usage(usage), _size(size), _mappedMemory(mappedData) {
}

Buffer::Buffer(Buffer&& buffer) noexcept
    : _buffer(std::exchange(buffer._buffer, VK_NULL_HANDLE)), _allocation(buffer._allocation), _logicalDevice(buffer._logicalDevice) {

}

Buffer& Buffer::operator=(Buffer&& buffer) noexcept {
    if (this == &buffer) {
        return *this;
    }
    // TODO what if _vertexBuffer != VK_NULL_HANDLE
    _buffer = std::exchange(buffer._buffer, VK_NULL_HANDLE);
    _allocation = buffer._allocation;
    _logicalDevice = buffer._logicalDevice;
}

Buffer::~Buffer() {
    if (_buffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _buffer }, _logicalDevice->getMemoryAllocator(), _allocation);
    }
}

VkBufferUsageFlags Buffer::getUsage() const {
    return _usage;
}

uint32_t Buffer::getSize() const {
    return _size;
}

void* Buffer::getMappedMemory() const {
    return _mappedMemory;
}