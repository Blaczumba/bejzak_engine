#include "orthographic_projection.h"

#include <glm/gtc/matrix_transform.hpp>

OrthographicProjection::OrthographicProjection(
    float left, float right, float bottom, float top, float nearZ, float farZ)
  : _left(left), _right(right), _bottom(bottom), _top(top), _zNear(nearZ), _zFar(farZ) {
  updateMatrix();
}

void OrthographicProjection::updateMatrix() {
  _matrix = glm::ortho(_left, _right, _bottom, _top, _zNear, _zFar);
  _matrix[1][1] = -_matrix[1][1];  // Flip Y for Vulkan-style clip space
}

void OrthographicProjection::setBounds(float left, float right, float bottom, float top) {
  _left = left;
  _right = right;
  _bottom = bottom;
  _top = top;
  updateMatrix();
}

void OrthographicProjection::setNear(float nearZ) {
  _zNear = nearZ;
  updateMatrix();
}

void OrthographicProjection::setFar(float farZ) {
  _zFar = farZ;
  updateMatrix();
}
