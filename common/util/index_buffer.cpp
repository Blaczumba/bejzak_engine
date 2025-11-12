#include "index_buffer.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>

namespace {

size_t getMaxIndex(std::span<const std::byte> indicesBuffer, size_t indexSize) {
  switch (indexSize) {
    case sizeof(uint32_t):
      {
        return *std::max_element(reinterpret_cast<const uint32_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint32_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint32_t));
      }
    case sizeof(uint16_t):
      {
        return *std::max_element(reinterpret_cast<const uint16_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint16_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint16_t));
      }
    case sizeof(uint8_t):
      {
        return *std::max_element(reinterpret_cast<const uint8_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint8_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint8_t));
      }
    default:
      {
        return *std::max_element(reinterpret_cast<const uint64_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint64_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint64_t));
      }
  }
  return 0;
}

}  // namespace

size_t getShrunkIndexSize(std::span<const std::byte> indicesBuffer, size_t indexSize) {
  const size_t maxIndex = getMaxIndex(indicesBuffer, indexSize);
  if (maxIndex <= std::numeric_limits<uint8_t>::max()) {
    return sizeof(uint8_t);
  } else if (maxIndex <= std::numeric_limits<uint16_t>::max()) {
    return sizeof(uint16_t);
  } else if (maxIndex <= std::numeric_limits<uint32_t>::max()) {
    return sizeof(uint32_t);
  } else {
    return sizeof(uint64_t);
  }
}

void copyAndShrinkIndices(void* dstIndices, size_t dstIndexSize, const void* srcIndices,
                          size_t srcIndexSize, size_t count) {
  std::byte* dstData = static_cast<std::byte*>(dstIndices);
  const std::byte* srcData = static_cast<const std::byte*>(srcIndices);
  for (size_t i = 0; i < count; i++) {
    std::memcpy(dstData, srcData, dstIndexSize);
    dstData += dstIndexSize;
    srcData += srcIndexSize;
  }
}
