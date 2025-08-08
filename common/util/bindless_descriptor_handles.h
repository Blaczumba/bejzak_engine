#pragma once

#include <cstdint>
#include <limits>

namespace {

using HandleType = uint32_t;

}  // namespace

enum class TextureHandle : HandleType {
  INVALID = std::numeric_limits<HandleType>::max()
};
enum class BufferHandle : HandleType {
  INVALID = std::numeric_limits<HandleType>::max()
};
