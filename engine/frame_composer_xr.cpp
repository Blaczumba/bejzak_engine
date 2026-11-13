#include "engine/frame_composer_xr.h"

namespace engine {

void FrameComposerXr::renderLayer(
    common::GraphicsContext& context, const LayerInfo& layerInfo, float deltaTime) {
  context.draw();
}

}  // namespace engine