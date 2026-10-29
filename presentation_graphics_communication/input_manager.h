#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tuple>
#include <utility>

#include "common/abstractions/presentation.h"

namespace engine {

struct InputConfig {
  bool obtainCursorInfo = false;
  bool obtainEyeTrackingInfo = false;
};

struct CursorInfo {
  uint32_t xoffset;
  uint32_t yoffset;
};

struct EyeTrackingInfo {
  glm::vec3 position;
  glm::quat orientation;
};

class InputManager {
public:
  InputManager(common::Presentation& presentation, const InputConfig& config);

  template <typename T>
  const T* get() const noexcept {
    return std::get<const T*>(_inputs);
  }

private:
  template <typename T>
  void registerIf(common::Presentation& presentation, bool enabled) {
    if (enabled) {
      std::get<const T*>(_inputs) = presentation.registerAction<T>();
    }
  }

  std::tuple<const CursorInfo*, const EyeTrackingInfo*> _inputs{};
};

}  // namespace engine
