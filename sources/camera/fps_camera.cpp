#include "fps_camera.h"

#include <glm/glm/gtc/constants.hpp>

#include <iostream>

FPSCamera::FPSCamera(float fovyRadians, float aspectRatio, float zNear, float zFar)
    : _fovy(fovyRadians), _aspectRatio(aspectRatio), _zNear(zNear), _zFar(zFar) {
    _projectionMatrix = glm::perspective(fovyRadians, aspectRatio, zNear, zFar);
    _projectionMatrix[1][1] = -_projectionMatrix[1][1];
    rotate(_yaw, _pitch);
}

void FPSCamera::updateInput(const CallbackData& cbData) {
    if (cbData.keyboardAction) {
        for (const auto direction : cbData.keys) {
            switch (direction) {
            case Keyboard::Key::W :
                move(cbData.deltaTime * _front);
                break;
            case Keyboard::Key::S :
                move(-cbData.deltaTime * _front);
                break;
            case Keyboard::Key::A :
                move(-cbData.deltaTime * _right);
                break;
            case Keyboard::Key::D :
                move(cbData.deltaTime * _right);
                break;
            }
        }
    }

    if (cbData.mouseAction) {
        _yaw += cbData.xoffset * _mouseSensitivity;
        _pitch += cbData.yoffset * _mouseSensitivity;
        _pitch = std::fminf(_pitch, glm::half_pi<float>() - glm::epsilon<float>()); // constrain pitch
        rotate(_yaw, _pitch);
    }
}

glm::mat4 FPSCamera::getViewMatrix() const {
    return glm::lookAt(_position, _position + _front, _up);
}

const glm::mat4& FPSCamera::getProjectionMatrix() const {
    return _projectionMatrix;
}

glm::vec3 FPSCamera::getPosition() const {
    return _position;
}

void FPSCamera::setAspectRatio(float aspectRatio) {
    _aspectRatio = aspectRatio;
    _projectionMatrix = glm::perspective(_fovy, _aspectRatio, _zNear, _zFar);
    _projectionMatrix[1][1] = -_projectionMatrix[1][1];
}

void FPSCamera::updateProjectionMatrix() {
    _projectionMatrix = glm::perspective(_fovy, _aspectRatio, _zNear, _zFar);
    _projectionMatrix[1][1] = -_projectionMatrix[1][1];
}

void FPSCamera::setFovy(float fovyRadians) {
    _fovy = fovyRadians;
    updateProjectionMatrix();
}

void FPSCamera::setZNear(float zNear) {
    _zNear = zNear;
    updateProjectionMatrix();
}

void FPSCamera::setZFar(float zFar) {
    _zFar = zFar;
    updateProjectionMatrix();
}

void FPSCamera::move(glm::vec3 direction) {
    _position += direction * _movementSpeed;
}

void FPSCamera::rotate(float theta, float phi) {
    _front.x = cos(theta) * cos(phi);
    _front.y = sin(phi);
    _front.z = sin(theta) * cos(phi);

    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}
