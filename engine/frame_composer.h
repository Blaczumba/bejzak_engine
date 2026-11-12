#pragma once

#include <stdfloat>
#include <vector>

#include "common/abstractions/graphics_context.h"

namespace engine {

enum class LayerType {
  GENERAL = 0,
  COMPOSITION_PROJECTION,
  COMPOSITION_QUAD,
};

struct Viewport {
  float xOffset;
  float yOffset;
  float width;
  float height;
};

struct LayerInfo {
  LayerType type;

  void* swapchain;
  uint32_t swapchainImageIndex;
  uint32_t firstLayer;
  uint32_t numLayers;
  Viewport viewport;

  uint32_t rendergraph;
};

class FrameComposer {
public:
  virtual void renderLayer(
      common::GraphicsContext& context, const LayerInfo& layerInfo, float deltaTime) = 0;
};

}  //  namespace engine
