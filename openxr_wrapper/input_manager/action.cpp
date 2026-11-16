#include "openxr_wrapper/input_manager/action.h"

#include <array>
#include <openxr/openxr.h>

#include "openxr_wrapper/util/check.h"
#include "openxr_wrapper/action/action.h"
#include "openxr_wrapper/space/space.h"

namespace xrw {
namespace {

enum class ViewConfigurationType : uint8_t {
  NONE = 0,
  MONO = 1,
  STEREO = 2,
  STEREO_WITH_FOVEATED_INSET = 4,
  MAX_VIEWS = STEREO_WITH_FOVEATED_INSET
};

size_t getViewCount(XrViewConfigurationType configurationType) {
  switch (configurationType) {
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
      return 1;
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
      return 2;
    case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
      return 4;
  }
  throw std::runtime_error("Configuration Type has not specified view count.");
}

glm::vec3 toGlm(const XrVector3f& v) {
  return {v.x, v.y, v.z};
}

glm::quat toGlm(const XrQuaternionf& q) {
  return {q.w, q.x, q.y, q.z};
}

common::Fov toCommonFov(const XrFovf& f) {
  return {f.angleLeft, f.angleRight, f.angleUp, f.angleDown};
}

}  // namespace

EyeProjectionAction::EyeProjectionAction(XrViewConfigurationType configurationType)
    : _configurationType(configurationType), _viewCount(getViewCount(configurationType)),
  _views(_viewCount) {
  assert(_viewCount <= static_cast<size_t>(ViewConfigurationType::MAX_VIEWS)
             && "Views count must be smaller than MAX_XR_VIEW_BUFFER_SIZE.");
}

void EyeProjectionAction::update(const Context& context) {
  assert(context.session != XR_NULL_HANDLE && "XrSession which is the input parameter of EyeTrackingAction update method cannot be XR_NULL_HANDLE.");
  assert(context.space != XR_NULL_HANDLE && "XrSpace which is the input parameter of EyeTrackingAction update method cannot be XR_NULL_HANDLE.");

  XrViewState viewState = {.type = XR_TYPE_VIEW_STATE};
  const XrViewLocateInfo viewLocateInfo = {
      .type = XR_TYPE_VIEW_LOCATE_INFO,
      .viewConfigurationType = _configurationType,
      .displayTime = context.predictedDisplayTime,
      .space = context.space};

  std::array<XrView, static_cast<size_t>(ViewConfigurationType::MAX_VIEWS)> views;
  for (uint32_t i = 0; i < _viewCount; i++) {
    views[i] = XrView{XR_TYPE_VIEW};
  }

  uint32_t viewCountOutput;
  CHECK_XRCMD(xrLocateViews(context.session, &viewLocateInfo, &viewState, _viewCount,
                            &viewCountOutput, views.data()), "Failed to xrLocateViews.");
  if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0
      || (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    if (_data.has_value()) {
      _views = std::move(_data->views);
      _data.reset();
    }
    return;
  }

  if (!_data.has_value()) {
    _data.emplace(common::EyeProjectionInfo{std::move(_views)});
  }
  for (uint32_t i = 0; i < _data->views.size(); i++) {
    _data->views[i] = common::EyeProjectionInfo::View{
      .position = toGlm(views[i].pose.position),
      .orientation = toGlm(views[i].pose.orientation),
      .fov = toCommonFov(views[i].fov)};
  }
}

const std::optional<common::EyeProjectionInfo>* EyeProjectionAction::getData() const noexcept {
  return &_data;
}

EyeTrackingAction::EyeTrackingAction(XrInstance instance, XrSession session, XrActionSet actionSet) {
  assert(instance != XR_NULL_HANDLE && "XrInstance which is the input parameter of EyeTrackingAction constructor cannot be XR_NULL_HANDLE.");
  assert(session != XR_NULL_HANDLE && "XrSession which is the input parameter of EyeTrackingAction constructor cannot be XR_NULL_HANDLE.");
  assert(actionSet != XR_NULL_HANDLE && "XrActionSet which is the input parameter of EyeTrackingAction constructor cannot be XR_NULL_HANDLE.");

  _eyeGazeAction =
  Action::create(actionSet, XR_ACTION_TYPE_POSE_INPUT, "eye-gaze-pose", "EyeGazePose");

  XrPath interactionProfilePath, gazePath;
  CHECK_XRCMD(xrStringToPath(instance, "/interaction_profiles/ext/eye_gaze_interaction",
                           &interactionProfilePath),
            "Failed to xrStringToPath.");
  CHECK_XRCMD(xrStringToPath(instance, "/user/eyes_ext/input/gaze_ext/pose", &gazePath),
            "Failed to xrStringToPath.");
  XrActionSuggestedBinding binding{.action = _eyeGazeAction.getAction(), .binding = gazePath};
  XrInteractionProfileSuggestedBinding suggestedBinding{
    .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
    .interactionProfile = interactionProfilePath,
    .countSuggestedBindings = 1,
    .suggestedBindings = &binding,
  };
  CHECK_XRCMD(xrSuggestInteractionProfileBindings(instance, &suggestedBinding),
            "Failed to xrSuggestInteractionProfileBindings.");
  XrSessionActionSetsAttachInfo attachInfo{
    .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
    .countActionSets = 1,
    .actionSets = &actionSet};
  CHECK_XRCMD(xrAttachSessionActionSets(session, &attachInfo), "Failed to attach action sets.");

  _actionSpace = Space::create(session, _eyeGazeAction.getAction(),
                             XrPosef{
                                 {0.f, 0.f, 0.f, 1.f},
                                 {}
                             });
}

void EyeTrackingAction::update(const Context& context) {
    if (!context.sessionFocused) {
      _data.reset();
      return;
    }
    assert(context.space != XR_NULL_HANDLE && "XrSpace which is the input parameter of EyeTrackingAction update method cannot be XR_NULL_HANDLE.");

    XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
    XrResult result = xrLocateSpace(
      _actionSpace->getXrSpace(), context.space, context.predictedDisplayTime, &spaceLocation);
    if ((spaceLocation.locationFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0
      || (spaceLocation.locationFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    _data.reset();
    return;
  }
  _data.emplace(toGlm(spaceLocation.pose.position), toGlm(spaceLocation.pose.orientation));
}

const std::optional<common::EyeTrackingInfo>* EyeTrackingAction::getData() const noexcept {
  return &_data;
}

}  // namespace xrw