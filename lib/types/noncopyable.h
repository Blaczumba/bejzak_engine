#pragma once

namespace lib {

class Noncopyable {
protected:
  Noncopyable() noexcept = default;

  ~Noncopyable() noexcept = default;

  Noncopyable(const Noncopyable&) = delete;

  Noncopyable& operator=(const Noncopyable&) = delete;
};

}  // namespace lib
