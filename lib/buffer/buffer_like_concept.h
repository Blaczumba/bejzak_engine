#pragma once

#include <concepts>

template <typename T>
concept BufferLike = requires (T t) {
  { t.data() } -> std::convertible_to<const void*>;
  { t.size() } -> std::convertible_to<std::size_t>;
};
