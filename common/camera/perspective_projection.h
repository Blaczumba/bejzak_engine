#pragma once

#include "projection.h"

class PerspectiveProjection : public Projection {
public:
    PerspectiveProjection(float fovy, float aspect, float nearZ, float farZ);
    void setFovy(float fovy);
    void setAspectRatio(float aspect);
    void setNear(float nearZ);
    void setFar(float farZ);
    void updateMatrix() override;
    const glm::mat4& getMatrix() const override;

private:
    float _fovy, _aspect, _zNear, _zFar;
    glm::mat4 _matrix;
};
