#pragma once

#include <openxr/openxr.h>
#include <optional>
#include <span>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "common/file/file_loader.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

class GraphicsPlugin {
public:
  virtual ~GraphicsPlugin() = default;

  virtual std::span<const char* const> getOpenXrInstanceExtensions() const = 0;

  virtual const XrBaseInStructure* getGraphicsBinding() const = 0;

  virtual std::optional<int64_t> selectSwapchainFormat(
      std::span<const int64_t> runtimeFormats) const = 0;

  virtual void createSwapchainContext(XrSwapchain swapchain, int64_t format, uint32_t width,
                                      uint32_t height, uint32_t layerCount) = 0;

  virtual common::PresentResources getSwapchainContext(XrSwapchain swapchain) = 0;

  virtual void initialize(XrInstance xrInstance, XrSystemId systemId) = 0;

  virtual void createResources() = 0;

  virtual std::unique_ptr<common::GraphicsContext> createGraphicsContext(
      XrInstance xrInstance, XrSystemId systemId,
      std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
      const FileLoader& fileLoader) = 0;
};

}  // namespace xrw
