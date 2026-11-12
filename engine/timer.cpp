#include "engine/timer.h"

namespace engine {

float Timer::tick() noexcept {
  const std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
  const auto deltaTime = std::chrono::duration<float>(current - _previous).count();
  _previous = current;
  return deltaTime;
}

}  // namespace engine
