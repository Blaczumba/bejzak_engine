#pragma once

#include <memory>
#include <span>

#include "common/abstractions/graphics_context.h"
#include "common/abstractions/presentation.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/instance/instance.h"
#include "openxr_wrapper/session/session.h"
#include "openxr_wrapper/space/space.h"
#include "openxr_wrapper/swapchain/swapchain.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

class Presentation final : public common::Presentation {
  Presentation(
      std::unique_ptr<Platform> platform, std::unique_ptr<GraphicsPlugin> graphicsPlugin,
               std::unique_ptr<common::GraphicsContext> graphicsContext,
               std::shared_ptr<engine::PresentationGraphicsCommunication>& communicationLayer,
      std::unique_ptr<Instance> instance, std::unique_ptr<System> system,
      std::unique_ptr<Session> session, std::vector<Swapchain>&& swapchains) noexcept;

public:
  static std::unique_ptr<common::Presentation> create(
      std::unique_ptr<Platform> platform, common::GraphicsApi graphicsApi,
      const FileLoader& fileLoader);

  ~Presentation() = default;

  common::GraphicsContext* getGraphicsContext() override;

  void run() override;

private:
  const XrEventDataBaseHeader* tryReadNextEvent();

  void handleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent);

  void pollEvents();

  void pollActions();

  // TODO: refactor
  bool renderLayer(XrTime predictedDisplayTime,
                   std::span<XrCompositionLayerProjectionView> projectionLayerViews,
                   XrCompositionLayerProjection& layer);

  void draw();

  static constexpr inline XrViewConfigurationType VIEW_CONFIG_TYPE =
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  std::unique_ptr<Platform> _platform;
  std::unique_ptr<common::GraphicsContext> _graphicsContext;
  std::shared_ptr<engine::PresentationGraphicsCommunication> _communicationLayer;
  std::unique_ptr<GraphicsPlugin> _graphicsPlugin;
  std::unique_ptr<Instance> _instance;
  std::unique_ptr<System> _system;
  std::unique_ptr<Session> _session;
  std::vector<Swapchain> _swapchains;
  std::unique_ptr<Space> _space;

  bool _sessionRunning = false;
  XrEventDataBuffer _eventDataBuffer;

  struct InputState {
    XrAction quitAction = XR_NULL_HANDLE;
  };

  InputState _input;
};

}  // namespace xrw
