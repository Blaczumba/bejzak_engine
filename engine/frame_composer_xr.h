#pragma once

#include "engine/frame_composer.h"

namespace engine {

class FrameComposerXr final : public FrameComposer {
public:
  void renderLayer(
      common::GraphicsContext& context, const LayerInfo& layerInfo, float deltaTime) override;
};

}  // namespace engine
