#pragma once

#include <array>
#include <memory>
#include <span>

#include "common/abstractions/graphics_context.h"
#include "common/abstractions/presentation.h"
#include "openxr_wrapper/action/action.h"
#include "openxr_wrapper/action/action_set.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/instance/instance.h"
#include "openxr_wrapper/session/session.h"
#include "openxr_wrapper/space/space.h"
#include "openxr_wrapper/swapchain/swapchain.h"
#include "presentation_graphics_communication/input_manager.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

enum class ViewConfigurationType : uint8_t {
  NONE = 0,
  MONO = 1,
  STEREO = 2,
  STEREO_WITH_FOVEATED_INSET = 4,
  MAX_VIEWS = STEREO_WITH_FOVEATED_INSET};

class PresentationAction {
public:
  struct Context {
    XrSession session = XR_NULL_HANDLE;
    XrSpace space = XR_NULL_HANDLE;
    XrTime predictedDisplayTime;
    bool sessionFocused = false;
  };

  virtual ~PresentationAction() = default;

  virtual void update(const Context& context) = 0;

  virtual const void* getData() const noexcept = 0;
};

class Presentation final : public common::Presentation {
  Presentation(std::unique_ptr<Platform> platform, std::unique_ptr<GraphicsPlugin> graphicsPlugin,
               std::unique_ptr<common::GraphicsContext> graphicsContext,
               std::shared_ptr<engine::PresentationGraphicsCommunication>& communicationLayer,
               std::unique_ptr<Instance> instance, std::unique_ptr<System> system,
               std::unique_ptr<Session> session, std::vector<Swapchain>&& swapchains);

public:
  static std::unique_ptr<common::Presentation> create(
      std::unique_ptr<Platform> platform, common::GraphicsApi graphicsApi,
      const FileLoader& fileLoader);

  ~Presentation() = default;

  common::GraphicsContext* getGraphicsContext() override;

  bool shouldClose() override;

  bool pollActionsAndEvents() override;

  bool beginFrame() override;

  void endFrame() override;

private:
  // TODO should not be compile time constant.
  static constexpr size_t VIEWS_NUM = 2;

  const void* registerActionImpl(std::type_index index) override;

  using PresentationActionFactory = std::function<std::unique_ptr<PresentationAction>()>;

  template <typename T>
  void addFactory(PresentationActionFactory factory, std::string_view extensionName) {
    assert(!_factories.contains(typeid(T)) && "The factor has already been added to the xrw::Presentation.");
    assert(!extensionName.empty());
    if (_instance->isExtensionEnabled(extensionName)) {
      _factories.emplace(typeid(T), std::move(factory));
    }
  }

  template <typename T>
  void addFactory(PresentationActionFactory factory) {
    assert(!_factories.contains(typeid(T)) && "The factor has already been added to the xrw::Presentation.");
    _factories.emplace(typeid(T), std::move(factory));
  }

  const XrEventDataBaseHeader* tryReadNextEvent();

  void handleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent);

  void pollEvents();

  void pollActions();

  bool renderProjectionLayer(
      std::span<XrCompositionLayerProjectionView> projectionLayerViews,
      XrCompositionLayerProjection& layer);

  std::vector<std::unique_ptr<PresentationAction>> _presentationActions;
  std::unordered_map<std::type_index, PresentationActionFactory> _factories;
  std::unordered_map<std::type_index, const void*> _actionInfos;

  const std::optional<engine::EyeProjectionInfo>* _eyeProjectionInfo = nullptr;
  const std::optional<engine::EyeTrackingInfo>* _eyeTrackingInfo = nullptr;

  // TODO: Should not be compile time constant.
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
  ActionSet _actionSet;

  bool _sessionFocused = false;
  bool _sessionRunning = false;
  XrEventDataBuffer _eventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};

  struct InputState {
    XrAction quitAction = XR_NULL_HANDLE;
  };

  InputState _input;

  // Transition to new presentation loop.
  struct LayerInfo {
    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::array<XrCompositionLayerProjectionView, VIEWS_NUM> layerViews = {
        XrCompositionLayerProjectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},
        XrCompositionLayerProjectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},
    };
  };
  LayerInfo _layerInfo;
  XrCompositionLayerBaseHeader* _layer;
  XrTime _predictedDisplayTime;
};

}  // namespace xrw
