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

// It is responsible for communication between main presentation and graphics context layers.
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

  void setScreenPos(uint32_t x, uint32_t y) noexcept;

  std::pair<uint32_t, uint32_t> getScreenPos() const noexcept;

private:
  uint32_t _swapchainImageIndex = 0;
  std::vector<common::CameraContext> _cameraContexts;
  std::pair<uint32_t, uint32_t> _screenPos = {};
};

} // engine
