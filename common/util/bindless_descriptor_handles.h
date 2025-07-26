#pragma once

#include <cstdint>
#include <limits>

enum class TextureHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };
enum class BufferHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };