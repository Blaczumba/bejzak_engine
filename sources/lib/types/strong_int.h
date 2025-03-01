#pragma once

#include <type_traits>

#define DEFINE_STRONG_INT(TYPE, INT_TYPE) using TYPE = lib::StrongInt<INT_TYPE, struct NAME##Tag>;

namespace lib {

template <typename T, typename Tag>
class StrongInt {
    static_assert(std::is_integral<T>::value, "StrongInt must be instantiated with an integer type");

    T value_;

public:
    explicit StrongInt(T value) : value_(value) {}

    T value() const { return value_; }

    explicit operator T() const { return value_; }

    bool operator==(const StrongInt& other) const { return value_ == other.value_; }
    bool operator!=(const StrongInt& other) const { return value_ != other.value_; }
    bool operator<(const StrongInt& other) const { return value_ < other.value_; }
    bool operator<=(const StrongInt& other) const { return value_ <= other.value_; }
    bool operator>(const StrongInt& other) const { return value_ > other.value_; }
    bool operator>=(const StrongInt& other) const { return value_ >= other.value_; }

    StrongInt& operator+=(const StrongInt& other) {
        value_ += other.value_;
        return *this;
    }
    StrongInt& operator-=(const StrongInt& other) {
        value_ -= other.value_;
        return *this;
    }
    StrongInt operator+(const StrongInt& other) const {
        return StrongInt(value_ + other.value_);
    }
    StrongInt operator-(const StrongInt& other) const {
        return StrongInt(value_ - other.value_);
    }
};

} // lib