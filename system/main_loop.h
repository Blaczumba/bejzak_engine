#pragma once

#include "common/abstractions/presentation.h"
#include "common/abstractions/asset_manager.h"
#include "common/abstractions/graphics_context.h"

namespace system {

void run(common::Presentation* presentation) {
  while (!presentation->shouldClose()) {
    presentation->pollActionsAndEvents();

    // The logic goes here.

    presentation->drawAndPresent();
  }
}

}  // namespace system
