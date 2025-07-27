#include "camera_impl.h"

#include <glm/gtc/matrix_transform.hpp>

CameraImpl::CameraImpl(const std::shared_ptr<Projection>& projection, glm::vec3 position, float moveSpeed, float mouseSensitivity)
	: _projection(projection), _position(position), _moveSpeed(moveSpeed), _mouseSensitivity(mouseSensitivity),
	_yaw(0.0f), _pitch(0.0f), _front{ 0.0f, 0.0f, -1.0f }, _up{ 0.0f, 1.0f, 0.0f },
    _right(glm::normalize(glm::cross(_front, glm::vec3(0.0f, 1.0f, 0.0f)))) {
}

glm::mat4 CameraImpl::getViewMatrix() const {
	return glm::lookAt(_position, _position + _front, _up);
}

const glm::mat4& CameraImpl::getProjectionMatrix() const {
	return _projection->getMatrix();
}

glm::vec3 CameraImpl::getPosition() const {
	return _position;
}

void CameraImpl::setPosition(const glm::vec3& pos) {
	_position = pos;
}

void CameraImpl::move(glm::vec3 direction) {
    _position += direction * _moveSpeed;
}

void CameraImpl::rotate(float theta, float phi) {
    _front.x = cos(theta) * cos(phi);
    _front.y = sin(phi);
    _front.z = sin(theta) * cos(phi);

    _right = glm::normalize(glm::cross(_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    _up = glm::normalize(glm::cross(_right, _front));
}

void CameraImpl::updateFromKeyboard(const MouseKeyboardManager& mouseKeyboardManager, float xOffset, float yOffset, float deltaTime) {
    if (mouseKeyboardManager.isPressed(Keyboard::Key::W)) {
        move(deltaTime * _front);
    }
    if (mouseKeyboardManager.isPressed(Keyboard::Key::S)) {
        move(-deltaTime * _front);
    }
    if (mouseKeyboardManager.isPressed(Keyboard::Key::A)) {
        move(-deltaTime * _right);
    }
    if (mouseKeyboardManager.isPressed(Keyboard::Key::D)) {
        move(deltaTime * _right);
    }

    _yaw += xOffset * _mouseSensitivity;
    _pitch += yOffset * _mouseSensitivity;
    _pitch = std::fminf(_pitch, glm::half_pi<float>() - glm::epsilon<float>()); // constrain pitch
    rotate(_yaw, _pitch);
}
