#pragma once

#include <span>

namespace common {

void copyData(std::span<std::byte> dst, std::span<const std::byte> src, size_t dstOffset = 0);

template <typename T>
void copyObject(std::span<std::byte> dst, const T& data, size_t dstOffset = 0) {
  copyData(dst, std::span(reinterpret_cast<const std::byte*>(&data), sizeof(data)), dstOffset);
}

}  // namespace common
