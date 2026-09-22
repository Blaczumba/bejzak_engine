#include "projection.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CalculateProjectionVisitor::operator()(const OrthographicProjection& ortho) {
  glm::mat4 matrix =
      glm::ortho(ortho.left, ortho.right, ortho.bottom, ortho.top, ortho.nearZ, ortho.farZ);
  matrix[1][1] = -matrix[1][1];  // Flip Y for Vulkan-style clip space.
  return matrix;
}

glm::mat4 CalculateProjectionVisitor::operator()(const PerspectiveProjection& persp) {
  glm::mat4 matrix = glm::perspective(persp.fovy, persp.aspect, persp.nearZ, persp.farZ);
  matrix[1][1] = -matrix[1][1];  // Flip Y for Vulkan-style clip space.
  return matrix;
}
