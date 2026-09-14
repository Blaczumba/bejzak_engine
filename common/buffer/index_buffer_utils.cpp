#include "common/buffer/index_buffer_utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace {

size_t getMaxIndex(std::span<const std::byte> indicesBuffer, IndexType indexSize) {
  switch (indexSize) {
    case IndexType::UINT32:
      {
        return *std::max_element(reinterpret_cast<const uint32_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint32_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint32_t));
      }
    case IndexType::UINT16:
      {
        return *std::max_element(reinterpret_cast<const uint16_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint16_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint16_t));
      }
    case IndexType::UINT8:
      {
        return *std::max_element(reinterpret_cast<const uint8_t*>(indicesBuffer.data()),
                                 reinterpret_cast<const uint8_t*>(indicesBuffer.data())
                                     + indicesBuffer.size() / sizeof(uint8_t));
      }
  }
  return *std::max_element(reinterpret_cast<const uint64_t*>(indicesBuffer.data()),
                           reinterpret_cast<const uint64_t*>(indicesBuffer.data())
                               + indicesBuffer.size() / sizeof(uint64_t));
}

}  // namespace

IndexType getShrunkIndexSize(std::span<const std::byte> indicesBuffer, IndexType indexSize) {
  return getCapableIndexType(getMaxIndex(indicesBuffer, indexSize));
}

IndexType getIndexType(uint8_t indexSize) noexcept {
  switch (indexSize) {
    case 1:
      return IndexType::UINT8;
    case 2:
      return IndexType::UINT16;
    case 4:
      return IndexType::UINT32;
    default:
      return IndexType::UINT64;
  }
}

IndexType getCapableIndexType(size_t maxIndex) noexcept {
  if (maxIndex <= std::numeric_limits<uint8_t>::max()) {
    return IndexType::UINT8;
  } else if (maxIndex <= std::numeric_limits<uint16_t>::max()) {
    return IndexType::UINT16;
  } else if (maxIndex <= std::numeric_limits<uint32_t>::max()) {
    return IndexType::UINT32;
  } else {
    return IndexType::UINT64;
  }
}
