#include "presentation_graphics_communication/presentation_graphics_communication.h"

#include <cstdint>
#include <vector>
#include <span>
#include <initializer_list>
#include <memory>

namespace engine {

std::unique_ptr<PresentationGraphicsCommunication> PresentationGraphicsCommunication::create() {
  return std::unique_ptr<PresentationGraphicsCommunication>(
      new PresentationGraphicsCommunication());
}

void PresentationGraphicsCommunication::setCurrentSwapchainImageIndex(
    uint32_t swapchainImageIndex) noexcept {
  _swapchainImageIndex = swapchainImageIndex;
}

uint32_t PresentationGraphicsCommunication::getCurrentSwapchainImageIndex() const noexcept {
  return _swapchainImageIndex;
}

void PresentationGraphicsCommunication::setCameraContexts(
    std::span<const common::CameraContext> cameraContexts) noexcept {
  _cameraContexts.assign_range(cameraContexts);
}

void PresentationGraphicsCommunication::setCameraContexts(
    std::initializer_list<common::CameraContext> cameraContexts) noexcept {
  _cameraContexts.assign_range(cameraContexts);
}

std::span<const common::CameraContext> PresentationGraphicsCommunication::getCameraContexts() const noexcept {
  return _cameraContexts;
}

void PresentationGraphicsCommunication::setScreenPos(uint32_t x, uint32_t y) noexcept {
  _screenPos = std::make_pair(x, y);
}

std::pair<uint32_t, uint32_t> PresentationGraphicsCommunication::getScreenPos() const noexcept {
  return _screenPos;
}

}  // namespace engine
