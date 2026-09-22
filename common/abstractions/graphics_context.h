#pragma once

#include <cstdint>

#include "common/abstractions/contexts.h"
#include "common/camera/camera.h"

namespace common {

enum class GraphicsApi : uint8_t {
  VULKAN = 0,
};

class GraphicsContext {
public:
  virtual ~GraphicsContext() = default;

  virtual void draw() = 0;

  virtual void initializeResources() = 0;

  virtual void waitCompleteExecution() const = 0;

  virtual void createPresentingResources(const PresentResources& presentResources) = 0;

  virtual void waitDeviceIdle() const = 0;

private:
};

}  // namespace common
