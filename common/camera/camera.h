#pragma once

#include <memory>

#include "camera.h"
#include "camera_controller.h"
#include "projection.h"

class Camera final : public MouseKeyboardCameraController {
public:
  Camera(const Projection& projection, glm::vec3 position, float moveSpeed, float mouseSensitivity);

  glm::mat4 getViewMatrix() const;

  const glm::mat4& getProjectionMatrix() const;

  Projection getProjection() const;

  void setProjection(const Projection& projection);

  glm::vec3 getPosition() const;

  void setPosition(const glm::vec3& pos);

  void updateFromKeyboard(
      const MouseKeyboardManager& mouseKeyboardManager, float deltaTime) override;

private:
  void move(glm::vec3 direction);

  void rotate(float theta, float phi);

  glm::vec3 _position;
  glm::vec3 _front;
  glm::vec3 _up;
  glm::vec3 _right;

  float _yaw;
  float _pitch;

  float _mouseXPos;
  float _mouseYPos;

  float _moveSpeed;
  float _mouseSensitivity;

  Projection _projection;
  glm::mat4 _projectionMatrix{1.0f};
};
