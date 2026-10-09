#include "common/buffer/index_buffer_lib.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <stdexcept>

namespace common {
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

// We use template here to let the compiler generate faster code for specific index sizes.
template <IndexType dstIndexSize, IndexType srcIndexSize>
void shrinkIndexDataImpl(std::byte* dstData, const std::byte* srcData, size_t indexCount) {
  for (size_t i = 0; i < indexCount; ++i) {
    std::memcpy(dstData, srcData, static_cast<size_t>(dstIndexSize));
    dstData += static_cast<size_t>(dstIndexSize);
    srcData += static_cast<size_t>(srcIndexSize);
  }
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
    default:
      return IndexType::UINT32;
  }
}

IndexType getCapableIndexType(size_t maxIndex) noexcept {
  if (maxIndex <= std::numeric_limits<uint8_t>::max()) {
    return IndexType::UINT8;
  } else if (maxIndex <= std::numeric_limits<uint16_t>::max()) {
    return IndexType::UINT16;
  } else {
    return IndexType::UINT32;
  }
}

void shrinkIndexData(std::span<std::byte> dst, std::span<const std::byte> src,
                     IndexType dstIndexSize, IndexType srcIndexSize) {
  const size_t indexCount = src.size() / static_cast<size_t>(srcIndexSize);
  assert(dst.size() / static_cast<size_t>(dstIndexSize) == indexCount);

  if (dstIndexSize == srcIndexSize) {
    std::memcpy(dst.data(), src.data(), src.size());
  } else if (dstIndexSize == IndexType::UINT8 && srcIndexSize == IndexType::UINT16) {
    shrinkIndexDataImpl<IndexType::UINT8, IndexType::UINT16>(dst.data(), src.data(), indexCount);
  } else if (dstIndexSize == IndexType::UINT8 && srcIndexSize == IndexType::UINT32) {
    shrinkIndexDataImpl<IndexType::UINT8, IndexType::UINT32>(dst.data(), src.data(), indexCount);
  } else if (dstIndexSize == IndexType::UINT16 && srcIndexSize == IndexType::UINT32) {
    shrinkIndexDataImpl<IndexType::UINT16, IndexType::UINT32>(dst.data(), src.data(), indexCount);
  } else {
    throw std::runtime_error(
        std::format("Unsupported index size conversion. dst: {}, src: {}.",
                    static_cast<size_t>(dstIndexSize), static_cast<size_t>(srcIndexSize)));
  }
}

}  // namespace common
