#pragma once

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>

#include "lib/types/util.h"

namespace lib {

template <typename TypeA, typename TypeB, size_t N>
class DualSparseMap {
public:
  using IndexType = typename SmallestIndex<N>::type;

  DualSparseMap() noexcept = default;

  ~DualSparseMap() = default;

  [[nodiscard]] TypeA* tryGetValueA(IndexType index);

  [[nodiscard]] const TypeA* tryGetValueA(IndexType index) const;

  [[nodiscard]] TypeB* tryGetValueB(IndexType index);

  [[nodiscard]] const TypeB* tryGetValueB(IndexType index) const;

  [[nodiscard]] std::tuple<const TypeA&, const TypeB&> getValue(IndexType index) const;

  [[nodiscard]] TypeA& getValueA(IndexType index);

  [[nodiscard]] const TypeA& getValueA(IndexType index) const;

  [[nodiscard]] TypeB& getValueB(IndexType index);

  [[nodiscard]] const TypeB& getValueB(IndexType index) const;

  [[nodiscard]] std::span<TypeA> getValuesA() noexcept;

  [[nodiscard]] std::span<const TypeA> getValuesA() const noexcept;

  [[nodiscard]] std::span<TypeB> getValuesB() noexcept;

  [[nodiscard]] std::span<const TypeB> getValuesB() const noexcept;

  [[nodiscard]] IndexType size() const noexcept;

  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] bool exists(IndexType index) const;

  bool insert(IndexType index, TypeA&& valueA, TypeB&& valueB);

  bool insert(IndexType index, const TypeA& valueA, const TypeB& valueB);

  void insertUnsafe(IndexType index, TypeA&& valueA, TypeB&& valueB);

  void insertUnsafe(IndexType index, const TypeA& valueA, const TypeB& valueB);

  bool erase(IndexType index);

  void eraseUnsafe(IndexType index);

private:
  IndexType _size = 0;
  std::array<IndexType, N> _sparse;
  std::array<IndexType, N> _dense;
  std::array<TypeA, N> _valuesA;
  std::array<TypeB, N> _valuesB;
};

template <typename TypeA, typename TypeB, size_t N>
TypeA* DualSparseMap<TypeA, TypeB, N>::tryGetValueA(IndexType index) {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_valuesA[denseIndex];
  }
  return nullptr;
}

template <typename TypeA, typename TypeB, size_t N>
const TypeA* DualSparseMap<TypeA, TypeB, N>::tryGetValueA(IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_valuesA[denseIndex];
  }
  return nullptr;
}

template <typename TypeA, typename TypeB, size_t N>
TypeA& DualSparseMap<TypeA, TypeB, N>::getValueA(IndexType index) {
  return _valuesA[_sparse[index]];
}

template <typename TypeA, typename TypeB, size_t N>
const TypeA& DualSparseMap<TypeA, TypeB, N>::getValueA(IndexType index) const {
  return _valuesA[_sparse[index]];
}

template <typename TypeA, typename TypeB, size_t N>
TypeB* DualSparseMap<TypeA, TypeB, N>::tryGetValueB(IndexType index) {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_valuesB[denseIndex];
  }
  return nullptr;
}

template <typename TypeA, typename TypeB, size_t N>
const TypeB* DualSparseMap<TypeA, TypeB, N>::tryGetValueB(IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  if (denseIndex < _size && _dense[denseIndex] == index) [[likely]] {
    return &_valuesB[denseIndex];
  }
  return nullptr;
}

template <typename TypeA, typename TypeB, size_t N>
TypeB& DualSparseMap<TypeA, TypeB, N>::getValueB(IndexType index) {
  return _valuesB[_sparse[index]];
}

template <typename TypeA, typename TypeB, size_t N>
const TypeB& DualSparseMap<TypeA, TypeB, N>::getValueB(IndexType index) const {
  return _valuesB[_sparse[index]];
}

template <typename TypeA, typename TypeB, size_t N>
std::tuple<const TypeA&, const TypeB&> DualSparseMap<TypeA, TypeB, N>::getValue(
    IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  return std::make_tuple(std::cref(_valuesA[denseIndex]), std::cref(_valuesB[denseIndex]));
}

template <typename TypeA, typename TypeB, size_t N>
std::span<TypeA> DualSparseMap<TypeA, TypeB, N>::getValuesA() noexcept {
  return std::span<TypeA>(_valuesA.data(), _size);
}

template <typename TypeA, typename TypeB, size_t N>
std::span<const TypeA> DualSparseMap<TypeA, TypeB, N>::getValuesA() const noexcept {
  return std::span<const TypeA>(_valuesA.data(), _size);
}

template <typename TypeA, typename TypeB, size_t N>
std::span<TypeB> DualSparseMap<TypeA, TypeB, N>::getValuesB() noexcept {
  return std::span<TypeB>(_valuesB.data(), _size);
}

template <typename TypeA, typename TypeB, size_t N>
std::span<const TypeB> DualSparseMap<TypeA, TypeB, N>::getValuesB() const noexcept {
  return std::span<const TypeB>(_valuesB.data(), _size);
}

template <typename TypeA, typename TypeB, size_t N>
typename DualSparseMap<TypeA, TypeB, N>::IndexType
DualSparseMap<TypeA, TypeB, N>::size() const noexcept {
  return _size;
}

template <typename TypeA, typename TypeB, size_t N>
bool DualSparseMap<TypeA, TypeB, N>::empty() const noexcept {
  return _size == 0;
}

template <typename TypeA, typename TypeB, size_t N>
bool DualSparseMap<TypeA, TypeB, N>::exists(IndexType index) const {
  const IndexType denseIndex = _sparse[index];
  return denseIndex < _size && _dense[denseIndex] == index;
}

template <typename TypeA, typename TypeB, size_t N>
bool DualSparseMap<TypeA, TypeB, N>::insert(IndexType index, TypeA&& valueA, TypeB&& valueB) {
  if (_size == N || exists(index)) [[unlikely]] {
    return false;
  }
  _sparse[index] = _size;
  _dense[_size] = index;
  _valuesA[_size] = std::move(valueA);
  _valuesB[_size] = std::move(valueB);
  ++_size;
  return true;
}

template <typename TypeA, typename TypeB, size_t N>
bool DualSparseMap<TypeA, TypeB, N>::insert(
    IndexType index, const TypeA& valueA, const TypeB& valueB) {
  if (_size == N || exists(index)) [[unlikely]] {
    return false;
  }
  _sparse[index] = _size;
  _dense[_size] = index;
  _valuesA[_size] = valueA;
  _valuesB[_size] = valueB;
  ++_size;
  return true;
}

template <typename TypeA, typename TypeB, size_t N>
void DualSparseMap<TypeA, TypeB, N>::insertUnsafe(IndexType index, TypeA&& valueA, TypeB&& valueB) {
  _sparse[index] = _size;
  _dense[_size] = index;
  _valuesA[_size] = std::move(valueA);
  _valuesB[_size] = std::move(valueB);
  ++_size;
}

template <typename TypeA, typename TypeB, size_t N>
void DualSparseMap<TypeA, TypeB, N>::insertUnsafe(
    IndexType index, const TypeA& valueA, const TypeB& valueB) {
  _sparse[index] = _size;
  _dense[_size] = index;
  _valuesA[_size] = valueA;
  _valuesB[_size] = valueB;
  ++_size;
}

template <typename TypeA, typename TypeB, size_t N>
bool DualSparseMap<TypeA, TypeB, N>::erase(IndexType index) {
  if (!exists(index)) [[unlikely]] {
    return false;
  }
  eraseUnsafe(index);
  return true;
}

template <typename TypeA, typename TypeB, size_t N>
void DualSparseMap<TypeA, TypeB, N>::eraseUnsafe(IndexType index) {
  const IndexType denseIndex = _sparse[index];
  const IndexType lastIndex = _dense[--_size];
  _dense[denseIndex] = lastIndex;

  if (_size != denseIndex) [[likely]] {
    _valuesA[denseIndex] = std::move(_valuesA[lastIndex]);
    _valuesB[denseIndex] = std::move(_valuesB[lastIndex]);
  } else {
    if constexpr (!std::is_trivially_destructible_v<TypeA>) {
      std::destroy_at(&_valuesA[denseIndex]);
    }
    if constexpr (!std::is_trivially_destructible_v<TypeB>) {
      std::destroy_at(&_valuesB[denseIndex]);
    }
  }
  _sparse[lastIndex] = denseIndex;
}

}  // namespace lib
