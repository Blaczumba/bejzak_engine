#include "buffer.h"

Buffer::Buffer() : _type(Buffer::Type::NONE), _buffer(VK_NULL_HANDLE), _size(0) {};

Buffer::Buffer(Type type, const VkBuffer buffer, const Allocation allocation, LogicalDevice& logicalDevice, uint32_t size, void* mappedMemory)
    : _type(type), _buffer(buffer), _allocation(allocation), _logicalDevice(&logicalDevice), _size(size), _mappedMemory(mappedMemory) {
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

Buffer::Type Buffer::getType() const {
    return _type;
}

uint32_t Buffer::getSize() const {
    return _size;
}

void* Buffer::getMappedMemory() const {
    return _mappedMemory;
}