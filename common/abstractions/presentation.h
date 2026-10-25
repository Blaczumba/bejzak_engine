#pragma once

#include <typeindex>

#include "common/abstractions/graphics_context.h"
#include "common/file/file_loader.h"

namespace common {

class Presentation {
public:
  virtual ~Presentation() = default;

  virtual GraphicsContext* getGraphicsContext() = 0;

  virtual void run() = 0;

  template<typename T>
  const T* registerAction() {
    return static_cast<const T*>(registerActionImpl(typeid(T)));
  }

 protected:
  virtual const void* registerActionImpl(std::type_index index) = 0;
};

}  // namespace common
