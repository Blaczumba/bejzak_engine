#pragma once

#include <memory>

#include "common/abstractions/graphics_context.h"
#include "common/abstractions/presentation.h"
#include "common/input_manager/mouse_keyboard_manager.h"
#include "common/window/window.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context.h"
#include "vulkan/wrapper/surface/surface.h"

namespace vlkn {

class Presentation final : public common::Presentation {
  Presentation(std::shared_ptr<Window> window, std::shared_ptr<Instance> instance,
               Surface&& surface, PresentationContext* presentationContext,
               std::unique_ptr<GraphicsContext<false, false>> graphicsContext,
               std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
               const FileLoader& fileLoader);

public:
  static std::unique_ptr<common::Presentation> create(
      std::shared_ptr<Window> window, const FileLoader& fileLoader);

  common::GraphicsContext* getGraphicsContext() override;

  ~Presentation() override = default;

  void run() override;

private:
  // TODO: Change to unique_ptr.
  std::shared_ptr<Window> _window;
  std::shared_ptr<engine::PresentationGraphicsCommunication> _communicationLayer;

  std::shared_ptr<Instance> _instance;
  Surface _surface;
  std::unique_ptr<GraphicsContext<false, false>> _graphicsContext;
  PresentationContext* _presentationContext;

  std::unique_ptr<const MouseKeyboardManager> _mouseKeyboardManager;
};

}  // namespace vlkn
