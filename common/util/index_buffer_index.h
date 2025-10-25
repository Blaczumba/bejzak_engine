#pragma once

#include <cstdint>
#include <limits>
#include <span>

constexpr uint8_t getMatchingIndexSize(size_t indicesCount) {
  if (indicesCount <= std::numeric_limits<uint8_t>::max()) {
    return 1;
  } else if (indicesCount <= std::numeric_limits<uint16_t>::max()) {
    return 2;
  } else if (indicesCount <= std::numeric_limits<uint32_t>::max()) {
    return 4;
  }
  return 0;
}

template <typename IndexT>
std::enable_if_t<std::is_unsigned<IndexT>::value> processIndices(
    void* outIndices, std::span<const IndexT> srcIndices, uint8_t indexSize) {
  std::byte* data = static_cast<std::byte*>(outIndices);
  for (const IndexT& index : srcIndices) {
    std::memcpy(data, &index, indexSize);
    data += indexSize;
  }
}
