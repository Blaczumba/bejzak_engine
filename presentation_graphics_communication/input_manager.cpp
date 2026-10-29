#include "presentation_graphics_communication/input_manager.h"

namespace engine {

InputManager::InputManager(common::Presentation& presentation, const InputConfig& config) {
  registerIf<CursorInfo>(presentation, config.obtainCursorInfo);
  registerIf<EyeTrackingInfo>(presentation, config.obtainEyeTrackingInfo);
}

}  // namespace engine
