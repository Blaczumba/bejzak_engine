#include "staging_buffer.h"

StagingBuffer::StagingBuffer(const VkBuffer buffer, const Allocation allocation, uint32_t size, MemoryAllocator& memoryAllocator)
    : _buffer(buffer), _allocation(allocation), _size(size), _memoryAllocator(memoryAllocator) {
}

StagingBuffer::~StagingBuffer() {
    std::visit(BufferDeallocator{ _buffer }, _memoryAllocator, _allocation);
}

const VkBuffer StagingBuffer::getVkBuffer() const {
    return _buffer;
}

uint32_t StagingBuffer::getSize() const {
    return _size;
}
