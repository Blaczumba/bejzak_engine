#pragma once

#include <openxr/openxr.h>
#include <string_view>

namespace xrw {

class ActionSet {
 public:
  ActionSet() noexcept = default;

  static ActionSet create(XrInstance instance, std::string_view actionSetName, std::string_view localizedActionSetName, uint32_t priority);

  ActionSet(ActionSet&& other) noexcept;

  ActionSet& operator=(ActionSet&& other) noexcept;

  ~ActionSet();

  XrActionSet getActionSet() const noexcept;

 private:
  ActionSet(XrActionSet actionSet) noexcept;

  XrActionSet _actionSet = XR_NULL_HANDLE;
};

}  // namespace xrw