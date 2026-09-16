#include "common/buffer/buffer_lib.h"

#include <span>

#include "common/util/engine_exception.h"

namespace common {

void copyData(std::span<std::byte> dst, std::span<const std::byte> src, size_t dstOffset) {
  if (dstOffset > dst.size() || dst.size() - dstOffset < src.size()) [[unlikely]] {
    throw EngineException(
        "Size of the destination buffer must be greater than source buffer size + offset.");
  }
  std::memcpy(dst.data() + dstOffset, src.data(), src.size());
}

}  // namespace common
