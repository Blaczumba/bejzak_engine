#pragma once

#include "memory_allocator/memory_allocator.h"
#include "memory_objects/buffer_deallocator.h"
#include "logical_device/logical_device.h"
#include "lib/buffer/buffer.h"
#include "lib/status/status.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <span>
#include <variant>
#include <vector>

namespace {

struct BufferDatas {
    VkBuffer buffer;
    Allocation allocation;
    void* data;
};

} // namespace

class StagingBuffer {
    StagingBuffer(const VkBuffer buffer, const Allocation allocation, uint32_t size, MemoryAllocator& memoryAllocator);

public:
    template <BufferLike Buffer>
    static lib::ErrorOr<std::unique_ptr<StagingBuffer>> create(MemoryAllocator& memoryAllocator, const Buffer& buffer);

    ~StagingBuffer();

    const VkBuffer getVkBuffer() const;
    uint32_t getSize() const;

private:
    VkBuffer _buffer;
    Allocation _allocation;
    uint32_t _size;

    MemoryAllocator& _memoryAllocator;

    struct Allocator {
        const size_t size;

        lib::ErrorOr<BufferDatas> operator()(VmaWrapper& wrapper) {
            ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, wrapper.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
            return BufferDatas{ buffer.buffer, buffer.allocation, buffer.mappedData };
        }

        lib::ErrorOr<BufferDatas> operator()(auto) {
            return lib::Error("Unrecognized allocator during StagingBuffer creation");
        }
    };
};

template <BufferLike Buffer>
lib::ErrorOr<std::unique_ptr<StagingBuffer>> StagingBuffer::create(MemoryAllocator& memoryAllocator, const Buffer& buffer) {
    const uint32_t size = buffer.size() * sizeof(std::decay_t<decltype(*buffer.data())>);
    ASSIGN_OR_RETURN(const BufferDatas bufferData, std::visit(Allocator{ size }, memoryAllocator));
    std::memcpy(bufferData.data, buffer.data(), size);
    return std::unique_ptr<StagingBuffer>(new StagingBuffer(bufferData.buffer, bufferData.allocation, size, memoryAllocator));
}
