#include "common/buffer/buffer_lib.h"

#include <cassert>
#include <span>

namespace common {

void copyData(std::span<std::byte> dst, std::span<const std::byte> src, size_t dstOffset) {
  assert(dstOffset <= dst.size() && dst.size() - dstOffset >= src.size());
  std::memcpy(dst.data() + dstOffset, src.data(), src.size());
}

}  // namespace common
