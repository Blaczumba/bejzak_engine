#include "common/buffer/buffer_utils.h"

#include <span>

#include "common/util/engine_exception.h"

namespace common {

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src,
              size_t srcOffset, size_t size) {
  if (dst.size() < size + dstOffset) [[unlikely]] {
    throw EngineException(
        "Size of the destination buffer must be greater than copied buffer size + offset.");
  }

  if (src.size() > size + srcOffset) [[unlikely]] {
    throw EngineException(
        "Size of the source buffer must be greater than copied buffer size + offset.");
  }

  std::memcpy(dst.data() + dstOffset, src.data() + srcOffset, size);
}

void copyData(std::span<std::byte> dst, size_t dstOffset, std::span<const std::byte> src) {
  copyData(dst, dstOffset, src, 0, src.size());
}

}  // namespace common
