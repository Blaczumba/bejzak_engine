#pragma once

#include <glm/glm.hpp>
#include <span>

#include "common/camera/camera.h"
#include "common/util/resource_handles.h"

namespace common {

struct CameraContext {
  glm::vec3 position;
  glm::mat4 view;
  glm::mat4 proj;
  glm::vec3 viewDir;
};

struct PresentResources {
  int64_t imageFormat;
  uint32_t width;
  uint32_t height;
  uint32_t numLayers;
  std::span<const std::byte> imageViews;  // Type erasure.
  bool multiview;
};

}  // namespace common
