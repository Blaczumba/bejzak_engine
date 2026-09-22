#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>

namespace lib {

template <size_t N>
struct SmallestIndex {
  using type = std::conditional_t<
      (N <= std::numeric_limits<uint8_t>::max() + 1), uint8_t,
      std::conditional_t<(N <= std::numeric_limits<uint16_t>::max() + 1), uint16_t, uint32_t>>;
};

template <typename To, typename From>
std::unique_ptr<To> dynamicUniqueCast(std::unique_ptr<From>&& p) {
  if (To* cast = dynamic_cast<To*>(p.get())) {
    std::unique_ptr<To> result(cast);
    p.release();
    return result;
  }
  return std::unique_ptr<To>(nullptr);
}

}  // namespace lib
