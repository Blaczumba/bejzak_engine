#include "action_set.h"

#include <cassert>
#include <cstring>
#include <string_view>
#include <utility>

#include "openxr_wrapper/util/check.h"

namespace xrw {

ActionSet::ActionSet(XrActionSet actionSet) noexcept : _actionSet(actionSet) {}

ActionSet ActionSet::create(XrInstance instance, std::string_view actionSetName,
                            std::string_view localizedActionSetName, uint32_t priority) {
  assert(instance != XR_NULL_HANDLE);
  assert(actionSetName.size() < XR_MAX_ACTION_SET_NAME_SIZE);
  assert(localizedActionSetName.size() < XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
  XrActionSetCreateInfo createInfo{.type = XR_TYPE_ACTION_SET_CREATE_INFO, .priority = priority};
  std::memcpy(createInfo.actionSetName, actionSetName.data(), actionSetName.size());
  std::memcpy(createInfo.localizedActionSetName, localizedActionSetName.data(),
              localizedActionSetName.size());
  XrActionSet actionSet;
  CHECK_XRCMD(
      xrCreateActionSet(instance, &createInfo, &actionSet), "Failed to create XrActionSet.");
  return ActionSet(actionSet);
}

ActionSet::ActionSet(ActionSet&& other) noexcept
  : _actionSet(std::exchange(other._actionSet, XR_NULL_HANDLE)) {}

ActionSet& ActionSet::operator=(ActionSet&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  if (_actionSet != XR_NULL_HANDLE) {
    xrDestroyActionSet(_actionSet);
  }

  _actionSet = std::exchange(other._actionSet, XR_NULL_HANDLE);
  return *this;
}

ActionSet::~ActionSet() {
  if (_actionSet != XR_NULL_HANDLE) {
    xrDestroyActionSet(_actionSet);
  }
}

XrActionSet ActionSet::getActionSet() const noexcept {
  return _actionSet;
}

}  // namespace xrw
