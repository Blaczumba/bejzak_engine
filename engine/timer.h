#pragma once

#include <chrono>

namespace engine {

class Timer {
public:
  Timer() noexcept = default;

  float tick() noexcept;

private:
  std::chrono::steady_clock::time_point _previous = std::chrono::steady_clock::now();
};

}  // namespace engine
