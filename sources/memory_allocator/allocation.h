#pragma once

#include "memory_allocator.h"

#include <vma/vk_mem_alloc.h>

#include <variant>

using MemoryAllocator = std::variant<std::monostate, VmaWrapper>;
using Allocation = std::variant<VmaAllocation>;
