#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "common/buffer/buffer.h"

enum class IndexType : uint8_t {
  UINT8 = 1,
  UINT16 = 2,
  UINT32 = 4,
  UINT64 = 8
};

IndexType getShrunkIndexSize(std::span<const std::byte> indicesBuffer, IndexType indexSize);

IndexType getIndexType(uint8_t indexSize);

IndexType getCapableIndexType(size_t maxIndex);
