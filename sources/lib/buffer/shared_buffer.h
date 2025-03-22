#pragma once

#include <algorithm>
#include <memory>

namespace lib {

    template<typename T>
    class SharedBuffer {
        std::shared_ptr<T[]> _buffer;
        size_t _size;

    public:
        SharedBuffer() : _size(0) {}
        explicit SharedBuffer(size_t size) : _buffer(std::make_shared_for_overwrite<T[]>(size)), _size(size) {}

        SharedBuffer(SharedBuffer& other) : _buffer(other._buffer), _size(other._size) {}

        template<typename Iterator>
        SharedBuffer(Iterator begin, Iterator end) : SharedBuffer(std::distance(begin, end)) {
            std::copy(begin, end, _buffer.get());
        }

        template<typename Iterator>
        SharedBuffer(Iterator begin, size_t n) : SharedBuffer(n) {
            std::copy(begin, std::next(begin, n), _buffer.get());
        }

        SharedBuffer(std::initializer_list<T> init) : SharedBuffer(init.size()) {
            std::copy(init.begin(), init.end(), _buffer.get());
        }

        SharedBuffer(SharedBuffer&& other) noexcept : _buffer(std::move(other._buffer)), _size(std::exchange(other._size, 0)) {}

        SharedBuffer& operator=(SharedBuffer& other) {
            if (this == &other) {
                return *this;
            }
            // TODO if sizes are equal then do not allocate memory
            _size = other._size;
            _buffer = other._buffer;
            std::copy(other._buffer.get(), other._buffer.get() + _size, _buffer.get());
            return *this;
        }

        SharedBuffer& operator=(SharedBuffer&& other) noexcept {
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