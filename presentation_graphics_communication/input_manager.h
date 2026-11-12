#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tuple>
#include <utility>

#include "common/abstractions/presentation.h"

namespace engine {

struct Fov {
  float angleLeft;
  float angleRight;
  float angleUp;
  float angleDown;
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

}  // namespace engine
