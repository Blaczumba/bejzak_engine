#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    virtual glm::mat4 getViewMatrix() const = 0;

    virtual const glm::mat4& getProjectionMatrix() const = 0;

    virtual glm::vec3 getPosition() const = 0;

	virtual void setPosition(const glm::vec3& position) = 0;

    virtual ~Camera() = default;
};


