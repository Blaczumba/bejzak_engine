#pragma once

#include <algorithm>
#include <memory>

namespace lib {

template<typename T>
class Buffer {
	std::unique_ptr<T[]> _buffer;
	size_t _size;

public:
	Buffer() : _size(0) {}
	explicit Buffer(size_t size) : _buffer(std::make_unique_for_overwrite<T[]>(size)), _size(size) {}

	Buffer(const Buffer& other) : Buffer(other._size) {
        // TODO if sizes are equal then do not allocate memory
		std::copy(other._buffer.get(), other._buffer.get() + _size, _buffer.get());
	}
    template<typename Iterator>
    Buffer(Iterator begin, Iterator end) : Buffer(std::distance(begin, end)) {
        std::copy(begin, end, _buffer.get());
    }
    template<typename Iterator>
    Buffer(Iterator begin, size_t n) : Buffer(n) {
        std::copy(begin, std::next(begin, n), _buffer.get());
    }
    Buffer(std::initializer_list<T> init) : Buffer(init.size()) {
        std::copy(init.begin(), init.end(), _buffer.get());
    }

	Buffer(Buffer&& other) noexcept : _buffer(std::move(other._buffer)), _size(std::exchange(other._size, 0)) {}

    Buffer& operator=(const Buffer& other) {
        if (this == &other) {
            return *this;
        }
        // TODO if sizes are equal then do not allocate memory
        _size = other._size;
        _buffer = std::make_unique_for_overwrite<T[]>(_size);
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

    T& operator[](size_t index) {
        return _buffer[index];
    }

    const T& operator[](size_t index) const {
        return _buffer[index];
    }

    bool empty() const {
        return _size == 0;
    }

    const T* data() const { return _buffer.get(); }
    T* data() { return _buffer.get(); }

    const T* begin() const { return _buffer.get(); }
    T* begin() { return _buffer.get(); }

    const T* end() const { return std::next(_buffer.get(), _size); }
    T* end() { return std::next(_buffer.get(), _size); }

    const T* cbegin() const { return _buffer.get(); }
    const T* cend() const { return std::next(_buffer.get(), _size); }

	size_t size() const { return _size; }
};

} // lib