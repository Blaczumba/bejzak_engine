#pragma once

#include <memory>
#include <openxr/openxr.h>
#include <optional>

#include "common/input_manager/xr_input_manager.h"
#include "openxr_wrapper/action/action.h"
#include "openxr_wrapper/space/space.h"

namespace xrw {

class InputAction {
public:
  struct Context {
    XrSession session = XR_NULL_HANDLE;
    XrSpace space = XR_NULL_HANDLE;
    XrTime predictedDisplayTime;
    bool sessionFocused = false;
  };

  virtual ~InputAction() = default;

  virtual void update(const Context& context) = 0;

  virtual const void* getData() const noexcept = 0;
};

class EyeProjectionAction final : public InputAction {
public:
  EyeProjectionAction(XrViewConfigurationType configurationType);

  ~EyeProjectionAction() = default;

  void update(const Context& context) override;

  const void* getData() const noexcept override;

private:
  std::optional<common::EyeProjectionInfo> _data;

  const XrViewConfigurationType _configurationType;
  const size_t _viewCount;
  // TODO: Change to std::inplace_vector.
  lib::Buffer<common::EyeProjectionInfo::View> _views;
};

class EyeTrackingAction final : public InputAction {
public:
  EyeTrackingAction(XrInstance instance, XrSession session, XrActionSet actionSet);

  ~EyeTrackingAction() = default;

  void update(const Context& context) override;

  const void* getData() const noexcept override;

private:
  std::optional<common::EyeTrackingInfo> _data;

  Action _eyeGazeAction;
  std::unique_ptr<Space> _actionSpace;
};

}  // namespace xrw
