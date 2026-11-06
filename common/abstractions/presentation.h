#pragma once

#include <typeindex>

#include "common/abstractions/graphics_context.h"
#include "common/file/file_loader.h"

namespace common {

class Presentation {
public:
  virtual ~Presentation() = default;

  virtual GraphicsContext* getGraphicsContext() = 0;

  template <typename T>
  const std::optional<T>* registerAction() {
    return reinterpret_cast<const std::optional<T>*>(registerActionImpl(typeid(T)));
  }

  virtual bool shouldClose() = 0;

  virtual bool pollActionsAndEvents() = 0;

  virtual bool beginFrame() = 0;

  virtual void endFrame() = 0;

protected:
  virtual const void* registerActionImpl(std::type_index index) = 0;
};

}  // namespace common
