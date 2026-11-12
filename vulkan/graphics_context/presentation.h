#pragma once

#include <memory>

#include "common/abstractions/graphics_context.h"
#include "common/abstractions/presentation.h"
#include "common/window/window.h"
#include "engine/system.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context.h"
#include "vulkan/wrapper/surface/surface.h"

namespace vlkn {

class Presentation final : public common::Presentation {
  Presentation(engine::System& system, std::shared_ptr<Window> window,
               std::shared_ptr<Instance> instance, std::shared_ptr<DebugMessenger> debugMessenger,
               Surface&& surface, std::shared_ptr<PhysicalDevice> physicalDevice,
               PhysicalDeviceOptionalExtendedFeatures optionalExtendedFeatures,
               std::shared_ptr<LogicalDevice> logicalDevice,
               std::shared_ptr<PresentationContext> presentationContext,
               std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer);

public:
  static std::unique_ptr<common::Presentation> create(
      engine::System& system, std::shared_ptr<Window> window);

  std::unique_ptr<common::GraphicsContext> createGraphicsContext() override;

  engine::PresentationGraphicsCommunication& getPresentationGraphicsCommunicationLayer() override;

  ~Presentation() override = default;

  bool shouldClose() override;

  bool pollActionsAndEvents() override;

  bool beginFrame() override;

  void endFrame() override;

private:
  const void* registerActionImpl(std::type_index index) override {
    return nullptr;
  }

  engine::System& _system;
  std::shared_ptr<Window> _window;
  std::shared_ptr<engine::PresentationGraphicsCommunication> _communicationLayer;

  std::shared_ptr<Instance> _instance;
  std::shared_ptr<DebugMessenger> _debugMessenger;
  Surface _surface;
  std::shared_ptr<PhysicalDevice> _physicalDevice;
  PhysicalDeviceOptionalExtendedFeatures _optionalExtendedFeatures;
  std::shared_ptr<LogicalDevice> _logicalDevice;
  std::shared_ptr<PresentationContext> _presentationContext;

  bool _graphicsContextCreated = false;
};

}  // namespace vlkn
