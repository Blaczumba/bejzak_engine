#pragma once

#include <cstdint>
#include <span>

size_t getShrunkIndexSize(std::span<const std::byte> indicesBuffer, size_t indexSize);

void copyAndShrinkIndices(void* dstIndices, size_t dstIndexSize, const void* srcIndices,
                          size_t srcIndexSize, size_t count);
