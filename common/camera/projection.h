#pragma once

#include <glm/glm.hpp>

class Projection {
public:
  virtual const glm::mat4& getMatrix() const = 0;

  virtual void updateMatrix() = 0;

  virtual ~Projection() = default;
};
