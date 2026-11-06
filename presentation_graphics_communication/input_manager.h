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

struct Fov {
  float angleLeft;
  float angleRight;
  float angleUp;
  float angleDown;
};

struct CursorInfo {
  uint32_t xoffset;
  uint32_t yoffset;
};

struct EyeTrackingInfo {
  glm::vec3 position;
  glm::quat orientation;
};

struct EyeProjectionInfo {
  struct View {
    glm::vec3 position;
    glm::quat orientation;
    Fov fov;
  };
  // TODO: Change to std::inplace_vector.
  lib::Buffer<View> views;
};

class InputManager {
public:
  InputManager(common::Presentation& presentation, const InputConfig& config);

  template <typename T>
  const std::optional<T>* get() const noexcept {
    return std::get<const std::optional<T>*>(_inputs);
  }

private:
  template <typename T>
  void registerIf(common::Presentation& presentation, bool enabled) {
    if (enabled) {
      std::get<const std::optional<T>*>(_inputs) = presentation.registerAction<T>();
    }
  }

  std::tuple<const std::optional<CursorInfo>*, const std::optional<EyeTrackingInfo>*> _inputs{};
};

}  // namespace engine
