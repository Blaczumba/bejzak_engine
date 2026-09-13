#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
#include <tuple>
#include <variant>
#include <vma/vk_mem_alloc.h>

#include "memory_allocator.h"

using MemoryAllocator = std::variant<VmaWrapper>;
using MemoryAllocatorPtr = std::unique_ptr<MemoryAllocator>;
using Allocation = std::variant<VmaAllocation>;

class VirtualBlock;

struct VirtualBlockMetadata {
  size_t size;
  std::byte* data;
};

struct VirtualAllocationMetadata {
  size_t offset;
  size_t size;
};

class VirtualAllocation {
  VirtualAllocation(
      const VirtualBlock& virtualBlock, std::variant<VmaVirtualAllocation> virtualAlloc) noexcept;

public:
  enum class Error : uint8_t {
    VIRTUAL_BLOCK_OUT_OF_MEMORY,
    INVALID_VIRTUAL_BLOCK
  };

  VirtualAllocation() = default;

  static std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, Error> create(
      const VirtualBlock& virtualBlock, size_t size, size_t alignment);

  VirtualAllocation(VirtualAllocation&& other) noexcept;

  VirtualAllocation& operator=(VirtualAllocation&& other) noexcept;

  ~VirtualAllocation();

  std::variant<VmaVirtualAllocation> getVkResource() const noexcept;

private:
  std::variant<VmaVirtualAllocation>& getVirtualAllocation() noexcept;

  const VirtualBlock* _virtualBlock = nullptr;
  std::variant<VmaVirtualAllocation> _virtualAlloc;

  friend class VirtualBlock;
};

class VirtualBlock {
  VirtualBlock(std::variant<std::monostate, VmaVirtualBlock>&& virtualBlock,
               std::unique_ptr<std::mutex> mutex) noexcept;

public:
  VirtualBlock() = default;

  static VirtualBlock create(const MemoryAllocator& memoryAllocator, size_t size);

  VirtualBlock(VirtualBlock&& other) noexcept;

  VirtualBlock& operator()(VirtualBlock&& other) noexcept;

  ~VirtualBlock();

  std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, VirtualAllocation::Error>
  createVirtualAllocation(size_t size, size_t alignment) const;

  bool empty() const;

private:
  void freeVirtualAllocation(VirtualAllocation& virtualAllocation) const;

  std::variant<std::monostate, VmaVirtualBlock> _virtualBlock;
  mutable std::unique_ptr<std::mutex> _mutex;

  friend class VirtualAllocation;
};
