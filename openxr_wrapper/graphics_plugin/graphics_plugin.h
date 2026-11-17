#pragma once

#include <openxr/openxr.h>
#include <optional>
#include <span>
#include <initializer_list>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "common/format.h"
#include "common/file/file_loader.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

class GraphicsPlugin {
public:
  virtual ~GraphicsPlugin() = default;

  virtual const XrBaseInStructure* getGraphicsBinding() const = 0;

  virtual std::optional<int64_t> selectFirstSupportedSwapchainFormat(XrSession session,
                                                                     std::initializer_list<common::Format> formats) const = 0;

  virtual std::optional<int64_t> isSupportedSwapchainFormat(XrSession session, common::Format format) const = 0;

  virtual void createSwapchainContext(XrSwapchain swapchain, int64_t format, uint32_t width,
                                      uint32_t height, uint32_t layerCount) = 0;

  virtual common::PresentResources getSwapchainContext(XrSwapchain swapchain) = 0;

  virtual void initialize(XrInstance xrInstance, XrSystemId systemId) = 0;

  virtual void createResources() = 0;

  virtual std::unique_ptr<common::GraphicsContext> createGraphicsContext(
      std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
      const FileLoader& fileLoader) = 0;
};

}  // namespace xrw
