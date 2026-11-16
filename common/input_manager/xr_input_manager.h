#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>

#include "lib/buffer/buffer.h"

namespace common {

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

// This class represents the interface for a widely supported set of functionalities across XR devices.
// In order to map more sophisticated headsets one should create the dedicated class which inheriths from this class.
class XrInputManager {
 public:
  virtual std::optional<EyeProjectionInfo>* getEyeProjectionInfo() const = 0;

  virtual std::optional<EyeTrackingInfo>* getEyeTrackingInfo() const = 0;
};

}  // namespace common
