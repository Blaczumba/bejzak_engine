#pragma once

#include <span>

namespace common {

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src,
              size_t srcOffset, size_t size);

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src);

template <typename T>
void copyData(std::span<std::byte> dst, size_t dstOffset, const T& data) {
  copyData(dst, dstOffset, std::span(reinterpret_cast<const std::byte*>(&data), sizeof(data)));
}

}  // namespace common
