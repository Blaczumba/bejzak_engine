#include "presentation_graphics_communication/presentation_graphics_communication.h"

#include <cstdint>
#include <span>
#include <initializer_list>
#include <memory>

namespace engine {

PresentationGraphicsCommunication::PresentationGraphicsCommunication(uint32_t numLayers) noexcept : _numLayers(numLayers) {}

std::unique_ptr<PresentationGraphicsCommunication> PresentationGraphicsCommunication::create(uint32_t numLayers) {
  return std::unique_ptr<PresentationGraphicsCommunication>(
      new PresentationGraphicsCommunication(numLayers));
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
  _cameraContexts.assign(cameraContexts);
}

std::span<const common::CameraContext> PresentationGraphicsCommunication::getCameraContexts() const noexcept {
  return _cameraContexts;
}

void PresentationGraphicsCommunication::setScreenPoses(std::initializer_list<std::pair<uint32_t, uint32_t>> screenPoses) {
  _screenPoses.assign(screenPoses);
}

void PresentationGraphicsCommunication::setScreenPoses(std::span<const std::pair<uint32_t, uint32_t>> screenPoses) {
  _screenPoses.assign_range(screenPoses);
}

std::span<const std::pair<uint32_t, uint32_t>> PresentationGraphicsCommunication::getScreenPoses() const noexcept {
  return _screenPoses;
}

void PresentationGraphicsCommunication::setMultiview(bool multiview) noexcept {
  _multiview = multiview;
}

bool PresentationGraphicsCommunication::isMultiviewEnabled() const noexcept {
  return _multiview && _numLayers > 1;
}

uint32_t PresentationGraphicsCommunication::getNumLayers() const noexcept {
  return _numLayers;
}

}  // namespace engine
