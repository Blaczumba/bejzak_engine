#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace common {

enum class IndexType : uint8_t {
  UINT8 = 1,
  UINT16 = 2,
  UINT32 = 4,
  UINT64 = 8
};

IndexType getShrunkIndexSize(std::span<const std::byte> indicesBuffer, IndexType indexSize);

IndexType getIndexType(uint8_t indexSize) noexcept;

IndexType getCapableIndexType(size_t maxIndex) noexcept;

void shrinkIndexData(std::span<std::byte> dst, std::span<const std::byte> src,
                            IndexType dstIndexSize, IndexType srcIndexSize);

}  // namespace common
