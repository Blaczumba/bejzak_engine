#pragma once

#include <array>
#include <memory>
#include <span>

#include "lib/types/util.h"

namespace lib {

template <typename Type, size_t N>
class SparseMap {
public:
  using IndexType = typename SmallestIndex<N>::type;

  SparseMap() noexcept = default;

  ~SparseMap() = default;

  [[nodiscard]] Type* tryGetValue(IndexType index);

  [[nodiscard]] const Type* tryGetValue(IndexType index) const;

  [[nodiscard]] Type& getValue(IndexType index);

  [[nodiscard]] const Type& getValue(IndexType index) const;

  [[nodiscard]] std::span<Type> getValues() noexcept;

  [[nodiscard]] std::span<const Type> getValues() const noexcept;

  [[nodiscard]] IndexType size() const noexcept;

  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] bool exists(IndexType index) const;

  bool insert(IndexType index, Type&& value);

  bool insert(IndexType index, const Type& value);

  Type& insertUnsafe(IndexType index, Type&& value);

  Type& insertUnsafe(IndexType index, const Type& value);

  bool erase(IndexType index);

  void eraseUnsafe(IndexType index);

private:
  IndexType _size = 0;
  std::array<IndexType, N> _sparse;
  std::array<IndexType, N> _dense;
  std::array<Type, N> _values;
};

template <typename Type, size_t N>
Type* SparseMap<Type, N>::tryGetValue(IndexType index) {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_values[denseIndex];
  }
  return nullptr;
}

template <typename Type, size_t N>
const Type* SparseMap<Type, N>::tryGetValue(IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_values[denseIndex];
  }
  return nullptr;
}

template <typename Type, size_t N>
Type& SparseMap<Type, N>::getValue(IndexType index) {
  return _values[_sparse[index]];
}

template <typename Type, size_t N>
const Type& SparseMap<Type, N>::getValue(IndexType index) const {
  return _values[_sparse[index]];
}

template <typename Type, size_t N>
std::span<Type> SparseMap<Type, N>::getValues() noexcept {
  return std::span<Type>(_values.data(), _size);
}

template <typename Type, size_t N>
std::span<const Type> SparseMap<Type, N>::getValues() const noexcept {
  return std::span<const Type>(_values.data(), _size);
}

template <typename Type, size_t N>
typename SparseMap<Type, N>::IndexType SparseMap<Type, N>::size() const noexcept {
  return _size;
}

template <typename Type, size_t N>
bool SparseMap<Type, N>::empty() const noexcept {
  return _size == 0;
}

template <typename Type, size_t N>
bool SparseMap<Type, N>::exists(IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  return denseIndex < _size && _dense[denseIndex] == index;
}

template <typename Type, size_t N>
bool SparseMap<Type, N>::insert(IndexType index, Type&& value) {
  if (_size == N || exists(index)) [[unlikely]] {
    return false;
  }
  _sparse[index] = _size;
  _dense[_size] = index;
  _values[_size] = std::move(value);
  ++_size;
  return true;
}

template <typename Type, size_t N>
bool SparseMap<Type, N>::insert(IndexType index, const Type& value) {
  if (_size == N || exists(index)) [[unlikely]] {
    return false;
  }
  _sparse[index] = _size;
  _dense[_size] = index;
  _values[_size++] = value;
  return true;
}

template <typename Type, size_t N>
Type& SparseMap<Type, N>::insertUnsafe(IndexType index, Type&& value) {
  _sparse[index] = _size;
  _dense[_size] = index;
  return _values[_size++] = std::move(value);
}

template <typename Type, size_t N>
Type& SparseMap<Type, N>::insertUnsafe(IndexType index, const Type& value) {
  _sparse[index] = _size;
  _dense[_size] = index;
  return _values[_size++] = value;
}

template <typename Type, size_t N>
bool SparseMap<Type, N>::erase(IndexType index) {
  if (!exists(index)) [[unlikely]] {
    return false;
  }
  eraseUnsafe(index);
  return true;
}

template <typename Type, size_t N>
void SparseMap<Type, N>::eraseUnsafe(IndexType index) {
  const IndexType denseIndex = _sparse[index];
  const IndexType lastIndex = _dense[--_size];
  _dense[denseIndex] = lastIndex;
  if (_size != denseIndex) [[likely]] {
    _values[denseIndex] = std::move(_values[lastIndex]);
  } else if constexpr (!std::is_trivially_destructible<Type>()) {
    std::destroy_at(&_values[denseIndex]);
  }
  _sparse[lastIndex] = denseIndex;
}

}  // namespace lib
