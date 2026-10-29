#include "engine/main_loop.h"

namespace engine {

void run(common::Presentation* presentation) {
  common::GraphicsContext* graphicsContext = presentation->getGraphicsContext();
  while (!presentation->shouldClose()) {
    if (!presentation->pollActionsAndEvents()) {
      continue;
    }

    graphicsContext->waitCompleteExecution();
    presentation->beginFrame();
    graphicsContext->draw();
    presentation->endFrame();
  }
}

}  // namespace engine
