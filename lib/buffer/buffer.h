#pragma once

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <span>
#include <utility>

namespace lib {

template <typename T>
class Buffer {
  std::unique_ptr<T[]> _buffer;
  size_t _size = 0;

public:
  Buffer() = default;

  ~Buffer() = default;

  explicit Buffer(size_t size)
    : _buffer(size > 0 ? std::make_unique_for_overwrite<T[]>(size) : nullptr), _size(size) {}

  Buffer(size_t size, T value) : Buffer(size) {
    std::fill(_buffer.get(), std::next(_buffer.get(), size), value);
  }

  Buffer(const Buffer& other) : Buffer(other._size) {
    std::copy(other._buffer.get(), other._buffer.get() + _size, _buffer.get());
  }

  template <typename Iterator>
  Buffer(Iterator begin, Iterator end) : Buffer(std::distance(begin, end)) {
    std::copy(begin, end, _buffer.get());
  }

  template <typename Iterator>
  Buffer(Iterator begin, size_t n) : Buffer(n) {
    std::copy(begin, std::next(begin, n), _buffer.get());
  }

  Buffer(std::initializer_list<T> init) : Buffer(init.size()) {
    std::copy(init.begin(), init.end(), _buffer.get());
  }

  Buffer(std::span<const T> buffer) : Buffer(std::cbegin(buffer), std::cend(buffer)) {}

  Buffer(Buffer&& other) noexcept
    : _buffer(std::move(other._buffer)), _size(std::exchange(other._size, 0)) {}

  Buffer& operator=(const Buffer& other) {
    if (this == &other) {
      return *this;
    }

    if (_size != other._size) {
      _size = other._size;
      _buffer = std::make_unique_for_overwrite<T[]>(_size);
    }
    std::copy(other._buffer.get(), other._buffer.get() + _size, _buffer.get());
    return *this;
  }

  Buffer& operator=(Buffer&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    _buffer = std::move(other._buffer);
    _size = std::exchange(other._size, 0);
    return *this;
  }

  Buffer& operator=(std::span<const T> other) noexcept {
    if (_size != other.size()) {
      _size = other.size();
      _buffer = std::make_unique_for_overwrite<T[]>(_size);
    }
    std::copy(other.begin(), other.end(), _buffer.get());
    return *this;
  }

  Buffer& operator=(std::initializer_list<T> other) noexcept {
    if (_size != other.size()) {
      _size = other.size();
      _buffer = std::make_unique_for_overwrite<T[]>(_size);
    }
    std::copy(other.begin(), other.end(), _buffer.get());
    return *this;
  }

  T& operator[](size_t index) {
    return _buffer[index];
  }

  const T& operator[](size_t index) const {
    return _buffer[index];
  }

  operator std::span<T>() noexcept {
    return std::span<T>(_buffer.get(), _size);
  }

  operator std::span<const T>() const noexcept {
    return std::span<const T>(_buffer.get(), _size);
  }

  bool empty() const noexcept {
    return _size == 0;
  }

  const T* data() const noexcept {
    return _buffer.get();
  }

  T* data() noexcept {
    return _buffer.get();
  }

  const T* begin() const noexcept {
    return _buffer.get();
  }

  T* begin() noexcept {
    return _buffer.get();
  }

  const T* end() const noexcept {
    return std::next(_buffer.get(), _size);
  }

  T* end() noexcept {
    return std::next(_buffer.get(), _size);
  }

  const T* cbegin() const noexcept {
    return _buffer.get();
  }

  const T* cend() const noexcept {
    return std::next(_buffer.get(), _size);
  }

  size_t size() const noexcept {
    return _size;
  }

  template <typename U>
  friend class SharedBuffer;
};

}  // namespace lib
