#pragma once

#include "engine/frame_composer.h"
#include "engine/input_manager.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace engine {

class FrameComposerDefault final : public FrameComposer {
public:
  FrameComposerDefault(
      PresentationGraphicsCommunication& communicationLayer, const InputManager& inputManager);

  void renderLayer(
      common::GraphicsContext& context, const LayerInfo& layerInfo, float deltaTime) override;

private:
  // TODO: This should not be here directly.
  PresentationGraphicsCommunication& _communicationLayer;
  const InputManager& _inputManager;
};

}  // namespace engine
