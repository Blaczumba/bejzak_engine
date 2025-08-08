#pragma once

#include "common/status/status.h"

#include <openxr/openxr.h>

#include <span>

namespace xrw {

class GraphicsPlugin {
 public:
  virtual ~GraphicsPlugin() = default;

  virtual std::span<const char *const> getOpenXrInstanceExtensions() const = 0;

  virtual const XrBaseInStructure *getGraphicsBinding() const = 0;

  virtual XrSwapchainImageBaseHeader *allocateSwapchainImageStructs(uint32_t capacity,
                                                                    const XrSwapchainCreateInfo &swapchain_create_info) = 0;

  virtual ErrorOr<int64_t> selectSwapchainFormat(std::span<const int64_t> runtimeFormats) const = 0;
};

} // xrw