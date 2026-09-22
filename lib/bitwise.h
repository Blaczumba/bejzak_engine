#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace lib {

template <typename T>
constexpr T setNLeastSignificantBits(uint8_t n) {
  static_assert(
      std::is_unsigned_v<T>, "lib::setNLeastSignificantBits<T> operates only on unsiged types.");
  if (n == 0) {
    return 0;
  }

  if (n >= sizeof(T) * 8) {
    return std::numeric_limits<T>::max();
  }

  return (1 << n) - 1;
}

}  // namespace lib
