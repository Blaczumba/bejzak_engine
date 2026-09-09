#include "vulkan/wrapper/memory_allocator/allocation.h"

#include <cstdint>
#include <expected>
#include <optional>
#include <tuple>
#include <variant>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/util/check.h"

namespace {

struct VirtualBlockInitializer {
  const size_t size;

  std::variant<std::monostate, VmaVirtualBlock> operator()(VmaAllocator allocator) {
    const VmaVirtualBlockCreateInfo createInfo{
      .size = size, .flags = VMA_VIRTUAL_BLOCK_CREATE_LINEAR_ALGORITHM_BIT};
    VmaVirtualBlock virtualBlock;
    CHECK_VKCMD(vmaCreateVirtualBlock(&createInfo, &virtualBlock),
                "Failed to allocate Virtual Block for a staging buffer - VMA.");
    return virtualBlock;
  }

  std::variant<std::monostate, VmaVirtualBlock> operator()(auto&&) {
    return std::monostate{};
  }
};

struct VirtualBlockDestroyer {
  void operator()(VmaVirtualBlock virtualBlock) {
    if (virtualBlock != VK_NULL_HANDLE) [[likely]] {
      vmaDestroyVirtualBlock(virtualBlock);
    }
  }

  void operator()(auto&&) {}
};

struct VirtualBlockEmpty {
  std::mutex& mutex;

  bool operator()(VmaVirtualBlock virtualBlock) {
    if (virtualBlock != VK_NULL_HANDLE) [[likely]] {
      std::lock_guard lck(mutex);
      return vmaIsVirtualBlockEmpty(virtualBlock);
    }
    return true;
  }

  bool operator()(auto&&) {
    return true;
  }
};

struct VirtualAllocator {
  std::mutex& mutex;

  const size_t size;
  const size_t alignment;

  std::optional<std::tuple<std::variant<VmaVirtualAllocation>, size_t>> operator()(
      VmaVirtualBlock virtualBlock) {
    if (virtualBlock == VK_NULL_HANDLE) [[unlikely]] {
      return std::nullopt;
    }

    VmaVirtualAllocationCreateInfo createInfo{.size = size, .alignment = alignment};

    VmaVirtualAllocation allocation;
    VkDeviceSize assignedOffset;

    {
      std::lock_guard lck(mutex);
      if (vmaVirtualAllocate(virtualBlock, &createInfo, &allocation, &assignedOffset) != VK_SUCCESS)
          [[unlikely]] {
        return std::nullopt;
      }
    }
    return std::make_tuple(allocation, assignedOffset);
  }

  std::optional<std::tuple<std::variant<VmaVirtualAllocation>, size_t>> operator()(auto&&) {
    return std::nullopt;
  }
};

struct VirtualDeallocator {
  std::mutex& mutex;

  void operator()(VmaVirtualBlock virtualBlock, VmaVirtualAllocation& allocation) {
    if (allocation != VK_NULL_HANDLE) {
      std::lock_guard lck(mutex);
      vmaVirtualFree(virtualBlock, allocation);
      allocation = VK_NULL_HANDLE;
    }
  }

  void operator()(auto&&, auto&&) {}
};

}  // namespace

VirtualAllocation::VirtualAllocation(
    const VirtualBlock& virtualBlock, std::variant<VmaVirtualAllocation> virtualAlloc) noexcept
  : _virtualBlock(&virtualBlock), _virtualAlloc(virtualAlloc) {}

std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, VirtualAllocation::Error>
VirtualAllocation::create(const VirtualBlock& virtualBlock, size_t size, size_t alignment) {
  return virtualBlock.createVirtualAllocation(size, alignment);
}

VirtualAllocation::VirtualAllocation(VirtualAllocation&& other) noexcept
  : _virtualBlock(std::exchange(other._virtualBlock, nullptr)), _virtualAlloc(other._virtualAlloc) {
}

VirtualAllocation& VirtualAllocation::operator=(VirtualAllocation&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  if (_virtualBlock != nullptr) [[unlikely]] {
    _virtualBlock->freeVirtualAllocation(*this);
  }
  _virtualBlock = std::exchange(other._virtualBlock, nullptr);
  _virtualAlloc = other._virtualAlloc;

  return *this;
}

VirtualAllocation::~VirtualAllocation() {
  if (_virtualBlock != nullptr) {
    _virtualBlock->freeVirtualAllocation(*this);
    _virtualBlock = nullptr;
  }
}

std::variant<VmaVirtualAllocation>& VirtualAllocation::getVirtualAllocation() noexcept {
  return _virtualAlloc;
}

VirtualBlock::VirtualBlock(std::variant<std::monostate, VmaVirtualBlock>&& virtualBlock,
                           std::unique_ptr<std::mutex> mutex) noexcept
  : _virtualBlock(std::move(virtualBlock)), _mutex(std::move(mutex)) {}

VirtualBlock VirtualBlock::create(const MemoryAllocator& memoryAllocator, size_t size) {
  return VirtualBlock(
      std::visit(VirtualBlockInitializer{size}, memoryAllocator), std::make_unique<std::mutex>());
}

VirtualBlock::VirtualBlock(VirtualBlock&& other) noexcept
  : _virtualBlock(std::exchange(other._virtualBlock, std::monostate{})),
    _mutex(std::move(other._mutex)) {}

VirtualBlock& VirtualBlock::operator()(VirtualBlock&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  std::visit(VirtualBlockDestroyer{}, _virtualBlock);

  _virtualBlock = std::exchange(other._virtualBlock, std::monostate{});
  _mutex = std::move(other._mutex);
  return *this;
}

VirtualBlock::~VirtualBlock() {
  std::visit(VirtualBlockDestroyer{}, _virtualBlock);
}

std::expected<std::tuple<VirtualAllocation, VirtualAllocationMetadata>, VirtualAllocation::Error>
VirtualBlock::createVirtualAllocation(size_t size, size_t alignment) const {
  std::optional<std::tuple<std::variant<VmaVirtualAllocation>, size_t>> virtualAllocation =
      std::visit(VirtualAllocator{*_mutex, size, alignment}, _virtualBlock);
  if (!virtualAllocation.has_value()) [[unlikely]] {
    return std::unexpected(VirtualAllocation::Error::VIRTUAL_BLOCK_OUT_OF_MEMORY);
  }
  return std::make_tuple(
      VirtualAllocation(*this, std::get<std::variant<VmaVirtualAllocation>>(*virtualAllocation)),
      VirtualAllocationMetadata{std::get<size_t>(*virtualAllocation), size});
}

bool VirtualBlock::empty() const {
  return std::visit(VirtualBlockEmpty{*_mutex}, _virtualBlock);
}

void VirtualBlock::freeVirtualAllocation(VirtualAllocation& virtualAllocation) const {
  std::visit(VirtualDeallocator{*_mutex}, _virtualBlock, virtualAllocation.getVirtualAllocation());
}
