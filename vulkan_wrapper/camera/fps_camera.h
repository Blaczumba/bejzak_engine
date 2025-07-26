#pragma once

#include "camera.h"

#include "vulkan_wrapper/window/window.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class FPSCamera : public Camera {
    glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 _worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 _front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 _right;

    float _yaw = 0.0f;
    float _pitch = 0.0f;
    float _movementSpeed = 2.5f;
    float _mouseSensitivity = 0.005f;
    float _fovy = glm::radians(45.0f);
    float _aspectRatio = 1.0f;
    float _zNear = 0.1f;
    float _zFar = 100.0f;

    glm::mat4 _projectionMatrix = glm::mat4(1.0f);

public:
    FPSCamera(float fovyRadians, float aspectRatio, float zNear, float zFar);

    void updateInput(const MouseKeyboardManager& inputManager, float xOffset, float yOffset, float deltaTime);

    void setAspectRatio(float aspectRatio);
    void setFovy(float fovyRadians);
    void setZNear(float zNear);
    void setZFar(float zFar);

    glm::mat4 getViewMatrix() const override;
    const glm::mat4& getProjectionMatrix() const;
    glm::vec3 getPosition() const;

    void move(glm::vec3 direction);
    void rotate(float theta, float phi);

private:
    void updateProjectionMatrix();
};
