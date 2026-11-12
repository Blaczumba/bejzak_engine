#include "engine/frame_composer_default.h"

#include "common/math/engine_math.h"

namespace engine {

FrameComposerDefault::FrameComposerDefault(
    PresentationGraphicsCommunication& communicationLayer, const InputManager& inputManager)
  : _communicationLayer(communicationLayer), _inputManager(inputManager) {}

void FrameComposerDefault::renderLayer(
    common::GraphicsContext& context, const LayerInfo& layerInfo, float deltaTime) {
  // To be removed:
  //_mouseKeyboardManager->setKeyboardCallback([&](Keyboard::Key key, int action) {
  //  switch (key) {
  //    case Keyboard::Key::Escape:
  //      _window->close();
  //      break;
  //  }
  //});

  static const auto [screenWidth, screenHeight] = std::tuple{1920, 1080};
  static Camera camera(PerspectiveProjection{glm::radians(45.0f), 1920.0f / 1080.f, 0.01f, 500.0f},
                       glm::vec3(0.0f), 5.5f, 0.01f);
  static glm::mat4 tempViewMat(1.0f);

  const MouseKeyboardManager& mouseKeyboardManager = *_inputManager.getMouseKeyboardManager();
  camera.updateFromKeyboard(mouseKeyboardManager, deltaTime);
  glm::vec2 mousePos = mouseKeyboardManager.getMousePosition();
  const glm::vec3 viewDir = common::getWorldSpaceViewDirection(
      mousePos.x, mousePos.y, screenWidth, screenHeight, glm::inverse(tempViewMat),
      glm::inverse(camera.getProjectionMatrix()));
  _communicationLayer.setCameraContexts({
    common::CameraContext{
                          camera.getPosition(),
                          mouseKeyboardManager.isPressed(Keyboard::Key::R) ? tempViewMat = camera.getViewMatrix() :
                                                         tempViewMat, camera.getProjectionMatrix(), viewDir}
  });
  _communicationLayer.setScreenPoses({
    {mousePos.x, mousePos.y}
  });
  context.draw();
}

}  // namespace engine
