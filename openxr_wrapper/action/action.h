#pragma once

#include <cassert>
#include <initializer_list>
#include <openxr/openxr.h>
#include <span>
#include <string_view>

namespace xrw {

class Action {
public:
  Action() noexcept = default;

  static Action create(
      XrActionSet actionSet, XrActionType actionType, std::string_view actionName,
      std::string_view localizedActionName, std::initializer_list<XrPath> subActionPaths = {});

  Action(Action&& other) noexcept;

  Action& operator=(Action&& other) noexcept;

  ~Action();

  XrAction getAction() const noexcept;

private:
  Action(XrAction action) noexcept;

  XrAction _action;
};

}  // namespace xrw
