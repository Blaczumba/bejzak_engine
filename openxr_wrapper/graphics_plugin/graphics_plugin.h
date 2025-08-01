#pragma once

#include <openxr/openxr.h>

#include <span>

namespace xrw {

class GraphicsPlugin {
 public:
  virtual std::span<const char *const> getOpenXrInstanceExtensions() const = 0;
  virtual const XrBaseInStructure *getGraphicsBinding() const = 0;

  virtual XrSwapchainImageBaseHeader *allocateSwapchainImageStructs(uint32_t capacity,
                                                                    const XrSwapchainCreateInfo &swapchain_create_info) = 0;

  virtual ~GraphicsPlugin() = default;
};

} // xrw