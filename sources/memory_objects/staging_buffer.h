#pragma once

#include "memory_allocator/memory_allocator.h"
#include "memory_objects/buffer_deallocator.h"
#include "logical_device/logical_device.h"
#include "lib/buffer/buffer.h"

#include <vulkan/vulkan.h>

#include <span>
#include <variant>
#include <vector>

class StagingBuffer {
public:
    template<typename Type>
    StagingBuffer(MemoryAllocator& memoryAllocator, const std::span<Type> buffer) : _memoryAllocator(memoryAllocator), _size(buffer.size() * sizeof(Type)) {
        std::tie(_buffer, _mappedData) = std::visit(Allocator{ _allocation, _size }, _memoryAllocator);
        std::memcpy(_mappedData, buffer.data(), _size);
    }

    template<typename Type>
    StagingBuffer(MemoryAllocator& memoryAllocator, const lib::Buffer<Type>& buffer) : _memoryAllocator(memoryAllocator), _size(buffer.size() * sizeof(Type)) {
        std::tie(_buffer, _mappedData) = std::visit(Allocator{ _allocation, _size }, _memoryAllocator);
        std::memcpy(_mappedData, buffer.data(), _size);
    }

    ~StagingBuffer();

    StagingBuffer(StagingBuffer&& stagingBuffer) noexcept;

    const VkBuffer getVkBuffer() const;
    uint32_t getSize() const;

private:
    VkBuffer _buffer;
    Allocation _allocation;
    uint32_t _size;
    void* _mappedData;
    MemoryAllocator& _memoryAllocator;

    struct Allocator {
        Allocation& allocation;
        const size_t size;

        std::pair<VkBuffer, void*> operator()(VmaWrapper& wrapper) {
            const auto[buffer, tmpallocation, data] = wrapper.createVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            allocation = tmpallocation;
            return std::make_pair(buffer, data);
        }

        std::pair<VkBuffer, void*> operator()(auto) {
            throw std::runtime_error("Unrecognized allocator during StagingBuffer creation");
        }
    };
};


