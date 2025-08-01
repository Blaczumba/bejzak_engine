#pragma once

#include "graphics_plugin.h"

#include <openxr/openxr.h>

#include <span>

namespace xrw {

class GraphicsPluginVulkan : public GraphicsPlugin {
 public:
  std::span<const char *const> getOpenXrInstanceExtensions() const override;
  const XrBaseInStructure *getGraphicsBinding() const override;

  XrSwapchainImageBaseHeader *allocateSwapchainImageStructs(uint32_t capacity,
                                                            const XrSwapchainCreateInfo &swapchain_create_info) override;

  ~GraphicsPluginVulkan() override = default;
};

} // xrw