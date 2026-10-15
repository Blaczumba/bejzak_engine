#pragma once

#include "common/abstractions/contexts.h"
#include "lib/types/noncopyable.h"

#include <cstdint>
#include <initializer_list>
#include <vector>
#include <span>
#include <memory>
#include <utility>

namespace engine {

// This class is responsible for communication between presentation and graphics layers.
// We assume that presentation layer and graphics layers run synchronously.
class PresentationGraphicsCommunication : private lib::Noncopyable {
  PresentationGraphicsCommunication() noexcept = default;

public:
  static std::unique_ptr<PresentationGraphicsCommunication> create();

  ~PresentationGraphicsCommunication() = default;

  void setCurrentSwapchainImageIndex(uint32_t swapchainImageIndex) noexcept;

  uint32_t getCurrentSwapchainImageIndex() const noexcept;

  void setCameraContexts(std::span<const common::CameraContext> cameraContexts) noexcept;

  void setCameraContexts(std::initializer_list<common::CameraContext> cameraContexts) noexcept;

  std::span<const common::CameraContext> getCameraContexts() const noexcept;

  void setScreenPoses(std::initializer_list<std::pair<uint32_t, uint32_t>> screenPoses);

  void setScreenPoses(std::span<const std::pair<uint32_t, uint32_t>> screenPoses);

  std::span<const std::pair<uint32_t, uint32_t>> getScreenPoses() const noexcept;

private:
  uint32_t _swapchainImageIndex = 0;
  std::vector<common::CameraContext> _cameraContexts;
  std::vector<std::pair<uint32_t, uint32_t>> _screenPoses;
};

} // engine
