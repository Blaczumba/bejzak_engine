#include "openxr_wrapper/presentation/presentation.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <span>
#include <spdlog/spdlog.h>

#include "common/camera/camera.h"
#include "common/file/file_loader.h"
#include "common/util/engine_exception.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin_vulkan.h"  // Needs to be included before platform.
#include "openxr_wrapper/util/check.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

namespace {

static constexpr size_t IMAGES_IN_LAYER = 2;

std::unique_ptr<GraphicsPlugin> createGraphicsPlugin(common::GraphicsApi graphicsApi) {
  switch (graphicsApi) {
    case common::GraphicsApi::VULKAN:
      return std::make_unique<GraphicsPluginVulkan>();
  }
}

glm::mat4 createProjectionMatrix(const XrFovf& fov, float near, float far) {
  const float tanLeft = tanf(fov.angleLeft);
  const float tanRight = tanf(fov.angleRight);
  const float tanDown = tanf(fov.angleDown);
  const float tanUp = tanf(fov.angleUp);

  const float tanWidth = tanRight - tanLeft;
  const float tanHeight = tanDown - tanUp;

  glm::mat4 result(0.0f);
  result[0][0] = 2.0f / tanWidth;
  result[1][1] = 2.0f / tanHeight;
  result[2][0] = (tanRight + tanLeft) / tanWidth;
  result[2][1] = (tanUp + tanDown) / tanHeight;
  result[2][2] = -far / (far - near);
  result[2][3] = -1.0f;
  result[3][2] = -(far * near) / (far - near);

  return result;
}

glm::mat4 createViewMatrix(const XrPosef& pose) {
  const glm::quat orientation =
      glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);

  const glm::vec3 position = glm::vec3(pose.position.x, pose.position.y, pose.position.z);

  return glm::translate(glm::mat4_cast(glm::conjugate(orientation)), -position);
}

}  // namespace

Presentation::Presentation(
    std::unique_ptr<Platform> platform, std::unique_ptr<GraphicsPlugin> graphicsPlugin,
    std::unique_ptr<common::GraphicsContext> graphicsContext,
    std::shared_ptr<engine::PresentationGraphicsCommunication>& communicationLayer,
    std::unique_ptr<Instance> instance, std::unique_ptr<System> system,
    std::unique_ptr<Session> session, std::vector<Swapchain>&& swapchains) noexcept
  : _platform(std::move(platform)), _graphicsPlugin(std::move(graphicsPlugin)),
    _graphicsContext(std::move(graphicsContext)), _communicationLayer(communicationLayer), _instance(std::move(instance)),
    _system(std::move(system)), _session(std::move(session)), _swapchains(std::move(swapchains)) {}

std::unique_ptr<common::Presentation> Presentation::create(
    std::unique_ptr<Platform> platform, common::GraphicsApi graphicsApi,
    const FileLoader& fileLoader) {
  std::unique_ptr<GraphicsPlugin> graphicsPlugin = createGraphicsPlugin(graphicsApi);
  std::unique_ptr<Instance> instance = Instance::create("BejzakEngine", *platform, *graphicsPlugin);
  std::unique_ptr<System> system = System::create(*instance);
  std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer =
      engine::PresentationGraphicsCommunication::create();
  std::unique_ptr<common::GraphicsContext> graphicsContext = graphicsPlugin->createGraphicsContext(
      instance->getXrInstance(), system->getXrSystemId(), communicationLayer, fileLoader);
  std::unique_ptr<Session> session = Session::create(*system, *graphicsPlugin);
  std::vector<Swapchain> swapchains =
      SwapchainBuilder()
          .withArraySize(2)
          .withViewConfigType(VIEW_CONFIG_TYPE)
          .build(*session, *graphicsPlugin);
  return std::unique_ptr<common::Presentation>(new Presentation(
      std::move(platform), std::move(graphicsPlugin), std::move(graphicsContext), communicationLayer,
      std::move(instance), std::move(system), std::move(session), std::move(swapchains)));
}

void Presentation::run() {
  for (const Swapchain& swapchain : _swapchains) {
    _graphicsContext->createPresentingResources(
        _graphicsPlugin->getSwapchainContext(swapchain.getSwapchain()));
  }
  _graphicsContext->initializeResources();
  _space = Space::create(_session->getXrSession(), XR_REFERENCE_SPACE_TYPE_LOCAL);
  while (!_platform->shouldClose()) {
    _platform->pollPlatformEvents(_sessionRunning);
    pollEvents();
    if (!_sessionRunning) {
      continue;
    }

    pollActions();
    draw();
  }
}

common::GraphicsContext* Presentation::getGraphicsContext() {
  return _graphicsContext.get();
}

const XrEventDataBaseHeader* Presentation::tryReadNextEvent() {
  auto baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&_eventDataBuffer);
  baseHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
  XrResult result = xrPollEvent(_instance->getXrInstance(), &_eventDataBuffer);
  if (result == XR_SUCCESS) {
    if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
      auto eventsLost = reinterpret_cast<XrEventDataEventsLost*>(baseHeader);
      spdlog::warn("{} events lost", eventsLost->lostEventCount);
    }
    return baseHeader;
  }
  if (result != XR_EVENT_UNAVAILABLE) {
    spdlog::error("xr pull event unknown result");
  }
  return nullptr;
}

void Presentation::handleSessionStateChangedEvent(
    const XrEventDataSessionStateChanged& stateChangedEvent) {
  if ((stateChangedEvent.session != XR_NULL_HANDLE)
      && (stateChangedEvent.session != _session->getXrSession())) {
    spdlog::error("XrEventDataSessionStateChanged for unknown session");
    return;
  }

  switch (stateChangedEvent.state) {
    case XR_SESSION_STATE_READY:
      {
        const XrSessionBeginInfo sessionBeginInfo = {
          .type = XR_TYPE_SESSION_BEGIN_INFO, .primaryViewConfigurationType = VIEW_CONFIG_TYPE};

        xrBeginSession(_session->getXrSession(), &sessionBeginInfo);
        _sessionRunning = true;
        break;
      }
    case XR_SESSION_STATE_STOPPING:
      {
        _sessionRunning = false;
        xrEndSession(_session->getXrSession());
        break;
      }
    default:
      break;
  }
}

void Presentation::pollEvents() {
  while (const XrEventDataBaseHeader* event = tryReadNextEvent()) {
    switch (event->type) {
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        {
          const auto& instanceLossPending =
              reinterpret_cast<const XrEventDataInstanceLossPending&>(*event);
          spdlog::warn("XrEventDataInstanceLossPending by {}", instanceLossPending.lossTime);
          return;
        }
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
          const auto& sessionStateChangedEvent =
              reinterpret_cast<const XrEventDataSessionStateChanged&>(*event);
          handleSessionStateChangedEvent(sessionStateChangedEvent);
          break;
        }
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        {
        }
        break;
      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      default:
        {
          spdlog::debug("Ignoring event type");
          break;
        }
    }
  }
}

void Presentation::pollActions() {
  const XrActionStateGetInfo getInfo = {
    .type = XR_TYPE_ACTION_STATE_GET_INFO,
    .next = nullptr,
    .action = _input.quitAction,
    .subactionPath = XR_NULL_PATH};

  XrActionStateBoolean quitValue = {.type = XR_TYPE_ACTION_STATE_BOOLEAN};

  // TODO: CHECK_XRCMD
  xrGetActionStateBoolean(_session->getXrSession(), &getInfo, &quitValue);
  if ((quitValue.isActive == XR_TRUE) && (quitValue.changedSinceLastSync == XR_TRUE)
      && (quitValue.currentState == XR_TRUE)) {
    // TODO: CHECK_XRCMD
    xrRequestExitSession(_session->getXrSession());
  }
}

bool Presentation::renderLayer(XrTime predictedDisplayTime,
                               std::span<XrCompositionLayerProjectionView> projectionLayerViews,
                               XrCompositionLayerProjection& layer) {
  XrViewState viewState = {.type = XR_TYPE_VIEW_STATE};

  const XrViewLocateInfo viewLocateInfo = {
    .type = XR_TYPE_VIEW_LOCATE_INFO,
    .viewConfigurationType = VIEW_CONFIG_TYPE,
    .displayTime = predictedDisplayTime,
    .space = _space->getXrSpace()};

  uint32_t viewCountOutput;
  std::array<XrView, IMAGES_IN_LAYER> views = {XrView{.type = XR_TYPE_VIEW}, XrView{.type = XR_TYPE_VIEW}};
  CHECK_XRCMD(xrLocateViews(_session->getXrSession(), &viewLocateInfo, &viewState, views.size(),
                            &viewCountOutput, views.data()),
              "Failed to xrLocateViews.");
  if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0
      || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    return false;  // There is no valid tracking poses
    // for the views.
  }

  const xrw::Swapchain& viewSwapchain = _swapchains[0];
  std::array<common::CameraContext, IMAGES_IN_LAYER> cameraContexts;
  for (uint32_t i = 0; i < IMAGES_IN_LAYER; i++) {
    const XrCompositionLayerProjectionView& projectionLayerView =
        projectionLayerViews[i] = XrCompositionLayerProjectionView{
          .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
          .pose = views[i].pose,
          .fov = views[i].fov,
          .subImage = {.swapchain = viewSwapchain.getSwapchain(),
                       .imageRect = {.offset = {0, 0}, .extent = viewSwapchain.getXrExtent2Di()},
                       .imageArrayIndex = i}
    };
    const auto [x, y, z] = projectionLayerView.pose.position;
    cameraContexts[i] = common::CameraContext{
      glm::vec3(x, y, z), createViewMatrix(projectionLayerView.pose),
      createProjectionMatrix(projectionLayerView.fov, 0.01f, 50.0f)};
  }
  _communicationLayer->setCameraContexts(cameraContexts);

  const XrSwapchainImageAcquireInfo acquireInfo = {.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

  uint32_t swapchainImageIndex;
  CHECK_XRCMD(
      xrAcquireSwapchainImage(viewSwapchain.getSwapchain(), &acquireInfo, &swapchainImageIndex),
      "Failed to xrAcquireSwapchainImage.");
  _communicationLayer->setCurrentSwapchainImageIndex(swapchainImageIndex);

  const XrSwapchainImageWaitInfo imageWaitInfo = {
    .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, .timeout = XR_INFINITE_DURATION};

  CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.getSwapchain(), &imageWaitInfo),
              "Failed to xrWaitSwapchainImage.");

  _graphicsContext->waitCompleteExecution();
  _graphicsContext->draw();

  const XrSwapchainImageReleaseInfo releaseInfo = {.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.getSwapchain(), &releaseInfo),
              "Failed to xrReleaseSwapchainImage.");

  layer.space = _space->getXrSpace();
  layer.viewCount = static_cast<uint32_t>(projectionLayerViews.size());
  layer.views = projectionLayerViews.data();
  return true;
}

void Presentation::draw() {
  if (_session->getXrSession() == XR_NULL_HANDLE) {
    throw EngineException("XrSession cannot be XR_NULL_HANDLE when rendering a frame.");
  }

  const XrFrameWaitInfo frameWaitInfo{
    .type = XR_TYPE_FRAME_WAIT_INFO,
  };

  XrFrameState frameState{
    .type = XR_TYPE_FRAME_STATE,
  };
  CHECK_XRCMD(
      xrWaitFrame(_session->getXrSession(), &frameWaitInfo, &frameState), "Failed to xrWaitFrame.");

  const XrFrameBeginInfo frameBeginInfo{
    .type = XR_TYPE_FRAME_BEGIN_INFO,
  };
  CHECK_XRCMD(xrBeginFrame(_session->getXrSession(), &frameBeginInfo), "Failed to xrBeginFrame.");

  XrCompositionLayerBaseHeader* layerHeader = nullptr;
  XrCompositionLayerProjection layer{
    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
  };
  std::array<XrCompositionLayerProjectionView, IMAGES_IN_LAYER> projectionLayerViews;
  if (frameState.shouldRender == XR_TRUE) {
    if (renderLayer(frameState.predictedDisplayTime, projectionLayerViews, layer)) {
      layerHeader = reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer);
    }
  }

  const XrFrameEndInfo frameEndInfo = {
    .type = XR_TYPE_FRAME_END_INFO,
    .displayTime = frameState.predictedDisplayTime,
    .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
    .layerCount = layerHeader != nullptr ? 1u : 0u,
    .layers = layerHeader != nullptr ? &layerHeader : nullptr};

  CHECK_XRCMD(xrEndFrame(_session->getXrSession(), &frameEndInfo), "Failed to xrEndFrame.");
}

}  // namespace xrw
