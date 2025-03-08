#include "staging_buffer.h"

StagingBuffer::~StagingBuffer() {
    if (_buffer != VK_NULL_HANDLE) {
        std::visit(BufferDeallocator{ _buffer }, _memoryAllocator, _allocation);
    }
}

StagingBuffer::StagingBuffer(StagingBuffer&& stagingBuffer) noexcept
    : _buffer(std::exchange(stagingBuffer._buffer, VK_NULL_HANDLE)), _allocation(stagingBuffer._allocation), _size(stagingBuffer._size),
    _mappedData(stagingBuffer._mappedData), _memoryAllocator(stagingBuffer._memoryAllocator) {
}

const VkBuffer StagingBuffer::getVkBuffer() const {
    return _buffer;
}

uint32_t StagingBuffer::getSize() const {
    return _size;
}