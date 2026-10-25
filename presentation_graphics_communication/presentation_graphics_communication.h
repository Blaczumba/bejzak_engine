#pragma once

#include "common/abstractions/contexts.h"
#include "lib/types/noncopyable.h"
#include "lib/buffer/buffer.h"

#include <cstdint>
#include <initializer_list>
#include <vector>
#include <span>
#include <memory>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine {

struct Viewport {
  uint32_t xOffset;
  uint32_t yOffset;
  uint32_t width;
  uint32_t height;
};

struct FrustumAsymmetrical {
  float leftAngle;
  float rightAngle;
  float upAngle;
  float bottomAngle;

  float nearZ;
  float farZ;
};

struct FrustumSymmetrical {
  float fovy;
  float aspectRatio;

  float nearZ;
  float farZ;
};

using Frustum = std::variant<FrustumAsymmetrical, FrustumSymmetrical>;

struct PresentationInfo {
  struct RenderView {
    glm::vec3 position;
    glm::quat orientation;
    Viewport viewport;
    Frustum frustum;
    uint32_t arrayLayer;
  };

  lib::Buffer<PresentationInfo> presentationInfos;
};

struct EyeTrackingInfo {
  glm::vec3 position;
  glm::quat orientation;
};

// This class is responsible for communication between presentation and graphics layers.
// We assume that presentation layer and graphics layers run synchronously.
class PresentationGraphicsCommunication : private lib::Noncopyable {
  PresentationGraphicsCommunication(uint32_t numLayers) noexcept;

public:
  static std::unique_ptr<PresentationGraphicsCommunication> create(uint32_t numLayers);

  ~PresentationGraphicsCommunication() = default;

  void setCurrentSwapchainImageIndex(uint32_t swapchainImageIndex) noexcept;

  uint32_t getCurrentSwapchainImageIndex() const noexcept;

  void setCameraContexts(std::span<const common::CameraContext> cameraContexts) noexcept;

  void setCameraContexts(std::initializer_list<common::CameraContext> cameraContexts) noexcept;

  std::span<const common::CameraContext> getCameraContexts() const noexcept;

  void setScreenPoses(std::initializer_list<std::pair<uint32_t, uint32_t>> screenPoses);

  void setScreenPoses(std::span<const std::pair<uint32_t, uint32_t>> screenPoses);

  std::span<const std::pair<uint32_t, uint32_t>> getScreenPoses() const noexcept;

  void setMultiview(bool multiview) noexcept;

  bool isMultiviewEnabled() const noexcept;

  uint32_t getNumLayers() const noexcept;

private:
  bool _multiview = false;
  uint32_t _numLayers = 0;
  uint32_t _swapchainImageIndex = 0;
  std::vector<common::CameraContext> _cameraContexts;
  std::vector<std::pair<uint32_t, uint32_t>> _screenPoses;
};

} // engine
