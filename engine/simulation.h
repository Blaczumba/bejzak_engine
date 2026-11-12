#pragma once

#include "common/abstractions/presentation.h"
#include "engine/frame_composer.h"
#include "engine/input_manager.h"
#include "engine/timer.h"

namespace engine {

class Simulation {
public:
  Simulation(common::Presentation& presentation, FrameComposer& frameComposer,
             InputManager& inputManager) noexcept;

  void run();

private:
  void update(float deltaTime);

  common::Presentation& _presentation;
  FrameComposer& _frameComposer;
  std::unique_ptr<common::GraphicsContext> _graphicsContext;

  InputManager& _inputManager;

  Timer _timer;
};

}  // namespace engine
