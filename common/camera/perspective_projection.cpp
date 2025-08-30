#include "perspective_projection.h"

#include <glm/gtc/matrix_transform.hpp>

PerspectiveProjection::PerspectiveProjection(float fovy, float aspect, float nearZ, float farZ)
  : _fovy(fovy), _aspect(aspect), _zNear(nearZ), _zFar(farZ) {
  updateMatrix();
}

void PerspectiveProjection::updateMatrix() {
  _matrix = glm::perspective(_fovy, _aspect, _zNear, _zFar);
  _matrix[1][1] = -_matrix[1][1];  // Flip Y for Vulkan-style clip space
}

void PerspectiveProjection::setFovy(float fovy) {
  _fovy = fovy;
  updateMatrix();
}

void PerspectiveProjection::setAspectRatio(float aspect) {
  _aspect = aspect;
  updateMatrix();
}

void PerspectiveProjection::setNear(float nearZ) {
  _zNear = nearZ;
  updateMatrix();
}

void PerspectiveProjection::setFar(float farZ) {
  _zFar = farZ;
  updateMatrix();
}
