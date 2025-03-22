#pragma once

#include "memory_allocator/memory_allocator.h"
#include "memory_objects/buffer_deallocator.h"
#include "logical_device/logical_device.h"
#include "lib/buffer/buffer.h"
#include "lib/status/status.h"

#include <vulkan/vulkan.h>

#include <span>
#include <variant>
#include <vector>

class StagingBuffer {
    StagingBuffer(const VkBuffer buffer, const Allocation allocation, uint32_t size, MemoryAllocator& memoryAllocator);

public:
    template <BufferLike Type>
    static lib::ErrorOr<std::unique_ptr<StagingBuffer>> create(MemoryAllocator& memoryAllocator, const Type& buffer);

    ~StagingBuffer();

    const VkBuffer getVkBuffer() const;
    uint32_t getSize() const;

private:
    VkBuffer _buffer;
    Allocation _allocation;
    uint32_t _size;

    MemoryAllocator& _memoryAllocator;

    struct Allocator {
        Allocation& allocation;
        const size_t size;

        lib::ErrorOr<std::pair<VkBuffer, void*>> operator()(VmaWrapper& wrapper) {
            ASSIGN_OR_RETURN(const VmaWrapper::Buffer buffer, wrapper.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
            allocation = buffer.allocation;
            return std::pair<VkBuffer, void*>(buffer.buffer, buffer.mappedData);
        }

        lib::ErrorOr<std::pair<VkBuffer, void*>> operator()(auto) {
            return lib::Error("Unrecognized allocator during StagingBuffer creation");
        }
    };
};

template <BufferLike Type>
static lib::ErrorOr<std::unique_ptr<StagingBuffer>> StagingBuffer::create(MemoryAllocator& memoryAllocator, const Type& buffer) {
    const uint32_t size = buffer.size() * sizeof(std::decay_t<decltype(*buffer.data())>);
    Allocation allocation;
    ASSIGN_OR_RETURN(const std::pair bufferData, std::visit(Allocator{ allocation, size }, memoryAllocator));
    std::memcpy(bufferData.second, buffer.data(), size);
    return std::unique_ptr<StagingBuffer>(new StagingBuffer(bufferData.first, allocation, size, memoryAllocator));
}


