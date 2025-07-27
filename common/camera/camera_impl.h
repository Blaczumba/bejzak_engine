#pragma once

#include "camera.h"

#include "camera_controller.h"
#include "projection.h"

#include <memory>

class CameraImpl final : public Camera, public MouseKeyboardCameraController {
public:
    CameraImpl(const std::shared_ptr<Projection>& projection, glm::vec3 position, float moveSpeed, float mouseSensitivity);

    glm::mat4 getViewMatrix() const override;

    const glm::mat4& getProjectionMatrix() const override;

    glm::vec3 getPosition() const override;

    void setPosition(const glm::vec3& pos) override;

	void updateFromKeyboard(const MouseKeyboardManager& mouseKeyboardManager, float xOffset, float yOffset, float deltaTime) override;

private:
    void move(glm::vec3 direction);

    void rotate(float theta, float phi);

    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;

    float _yaw;
	float _pitch;

    float _moveSpeed;
	float _mouseSensitivity;

    std::shared_ptr<Projection> _projection;
};