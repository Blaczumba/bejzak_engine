#pragma once

#include "projection.h"

class OrthographicProjection : public Projection {
public:
  OrthographicProjection(float left, float right, float bottom, float top, float nearZ, float farZ);

  ~OrthographicProjection() override = default;

  void updateMatrix() override;

  void setBounds(float left, float right, float bottom, float top);

  void setNear(float nearZ);

  void setFar(float farZ);

private:
  float _left, _right, _bottom, _top, _zNear, _zFar;
};
