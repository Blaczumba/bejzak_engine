#pragma once

#include "projection.h"

class PerspectiveProjection : public Projection {
public:
  PerspectiveProjection(float fovy, float aspect, float nearZ, float farZ);

  ~PerspectiveProjection() override = default;

  void updateMatrix() override;

  void setFovy(float fovy);

  void setAspectRatio(float aspect);

  void setNear(float nearZ);

  void setFar(float farZ);

private:
  float _fovy, _aspect, _zNear, _zFar;
};
