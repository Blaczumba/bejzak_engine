#pragma once

#include <glm/glm.hpp>
#include <variant>

struct OrthographicProjection {
  float left, right, bottom, top, nearZ, farZ;
};

struct PerspectiveProjection {
  float fovy, aspect, nearZ, farZ;
};

using Projection = std::variant<OrthographicProjection, PerspectiveProjection>;

struct CalculateProjectionVisitor {
  glm::mat4 operator()(const OrthographicProjection& ortho);

  glm::mat4 operator()(const PerspectiveProjection& persp);
};
