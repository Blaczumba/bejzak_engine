#include "openxr_wrapper/input_manager/xr_input_manager.h"

#include "openxr_wrapper/util/check.h"

namespace xrw {

XrInputManager::XrInputManager(XrInstance instance, XrSession session, XrViewConfigurationType viewConfigType)
  : _instance(instance), _viewConfigType(viewConfigType) {
  _actionSet = ActionSet::create(_instance, "eye-gaze", "EyeGaze", 1000);
}

std::optional<common::EyeProjectionInfo>* XrInputManager::getEyeProjectionInfo() const {

}

std::optional<common::EyeTrackingInfo>* XrInputManager::getEyeTrackingInfo() const {

}

void XrInputManager::pollActions(std::optional<XrSession> session) const {
  if (!session.has_value()) {
    return;
  }

  const XrActiveActionSet activeActionSet{
    .actionSet = _actionSet.getActionSet(), .subactionPath = XR_NULL_PATH};
  XrActionsSyncInfo syncInfo{.type = XR_TYPE_ACTIONS_SYNC_INFO,
                             .countActiveActionSets = 1,
                             .activeActionSets = &activeActionSet};
  CHECK_XRCMD(xrSyncActions(*session, &syncInfo), "Failed to sync actions.");
}

void XrInputManager::update(const InputAction::Context& context) {
  for (std::unique_ptr<InputAction>& inputAction : _inputActions) {
    inputAction->update(context);
  }
}

void XrInputMaregisterActionImpl(std::type_index typeIndex) {

}

}  // namespace xrw