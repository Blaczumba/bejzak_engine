#pragma once

#include <variant>
#include <vma/vk_mem_alloc.h>

#include "memory_allocator.h"

using MemoryAllocator = std::variant<std::monostate, VmaWrapper>;
using Allocation = std::variant<std::monostate, VmaAllocation>;
