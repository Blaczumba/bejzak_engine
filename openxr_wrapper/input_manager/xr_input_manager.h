#pragma once

#include <typeindex>
#include <unordered_map>
#include <vector>

#include "common/input_manager/xr_input_manager.h"
#include "openxr_wrapper/action/action_set.h"
#include "openxr_wrapper/input_manager/action.h"

namespace xrw {

class XrInputManager final : public common::XrInputManager {
public:
  XrInputManager(XrInstance instance, XrSession session, XrViewConfigurationType viewConfigType);

  std::optional<common::EyeProjectionInfo>* getEyeProjectionInfo() const override;

  std::optional<common::EyeTrackingInfo>* getEyeTrackingInfo() const override;

  void pollActions(std::optional<XrSession>) const;

  void update(const InputAction::Context& context);

private:
  ActionSet _actionSet;
  XrInstance _instance;
  XrSession _session;
  XrViewConfigurationType _viewConfigType;

  std::vector<std::unique_ptr<InputAction>> _inputActions;
  std::unordered_map<std::type_index, PresentationActionFactory> _factories;
  std::unordered_map<std::type_index, const void*> _actionInfos;

  std::optional<common::EyeProjectionInfo>* _eyeProjectionInfo = nullptr;
  std::optional<common::EyeTrackingInfo>* _eyeTrackingInfo = nullptr;
};

}  // namespace xrw
