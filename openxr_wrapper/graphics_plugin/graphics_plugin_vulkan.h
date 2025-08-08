#pragma once

#include <openxr/openxr.h>
#include <span>

#include "common/status/status.h"
#include "graphics_plugin.h"

namespace xrw {

class GraphicsPluginVulkan : public GraphicsPlugin {
public:
  ~GraphicsPluginVulkan() override = default;

  std::span<const char* const> getOpenXrInstanceExtensions() const override;

  const XrBaseInStructure* getGraphicsBinding() const override;

  XrSwapchainImageBaseHeader* allocateSwapchainImageStructs(
      uint32_t capacity, const XrSwapchainCreateInfo& swapchain_create_info) override;

  ErrorOr<int64_t> selectSwapchainFormat(std::span<const int64_t> runtimeFormats) const override;
};

}  // namespace xrw
