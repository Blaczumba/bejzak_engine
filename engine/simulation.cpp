#include "engine/simulation.h"

#include "common/abstractions/presentation.h"

namespace engine {

Simulation::Simulation(common::Presentation& presentation, FrameComposer& frameComposer,
                       InputManager& inputManager) noexcept
  : _presentation(presentation), _frameComposer(frameComposer),
    _graphicsContext(_presentation.createGraphicsContext()), _inputManager(inputManager) {}

void Simulation::run() {
  while (!_presentation.shouldClose()) {
    if (!_presentation.pollActionsAndEvents()) {
      continue;
    }

    const float deltaTime = _timer.tick();
    update(deltaTime);

    _graphicsContext->waitCompleteExecution();
    if (_presentation.beginFrame()) {
      _frameComposer.renderLayer(*_graphicsContext, {}, deltaTime);
    }
    _presentation.endFrame();
  }
}

void Simulation::update(float deltaTime) {}

}  // namespace engine
