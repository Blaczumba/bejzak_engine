#include "action.h"

#include <cassert>
#include <cstring>
#include <initializer_list>
#include <openxr/openxr.h>
#include <utility>

#include "openxr_wrapper/util/check.h"

namespace xrw {

Action::Action(XrAction action) noexcept : _action(action) {}

Action Action::create(
    XrActionSet actionSet, XrActionType actionType, std::string_view actionName,
    std::string_view localizedActionName, std::initializer_list<XrPath> subActionPaths) {
  assert(actionSet != XR_NULL_HANDLE);
  assert(actionName.size() < XR_MAX_ACTION_NAME_SIZE);
  assert(localizedActionName.size() < XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
  XrActionCreateInfo createInfo{
    .type = XR_TYPE_ACTION_CREATE_INFO,
    .actionType = actionType,
    .countSubactionPaths = static_cast<uint32_t>(subActionPaths.size()),
    .subactionPaths = !empty(subActionPaths) ? subActionPaths.begin() : nullptr};
  std::memcpy(createInfo.actionName, actionName.data(), actionName.size());
  std::memcpy(
      createInfo.localizedActionName, localizedActionName.data(), localizedActionName.size());
  XrAction action;
  CHECK_XRCMD(xrCreateAction(actionSet, &createInfo, &action), "Failed to create XrAction.");
  return Action(action);
}

Action::Action(Action&& other) noexcept : _action(std::exchange(other._action, XR_NULL_HANDLE)) {}

Action& Action::operator=(Action&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  if (_action != XR_NULL_HANDLE) {
    xrDestroyAction(_action), "Failed to destroy XrAction.";
  }

  _action = std::exchange(other._action, XR_NULL_HANDLE);
  return *this;
}

Action::~Action() {
  if (_action != XR_NULL_HANDLE) {
    xrDestroyAction(_action);
  }
}

XrAction Action::getAction() const noexcept {
  return _action;
}

}  // namespace xrw
