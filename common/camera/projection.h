#pragma once

#include <glm/glm.hpp>

class Projection {
public:
  virtual ~Projection() = default;

  const glm::mat4& getMatrix() const {
    return _matrix;
  }

  virtual void updateMatrix() = 0;

protected:
  glm::mat4 _matrix = glm::mat4(1.0f);
};
