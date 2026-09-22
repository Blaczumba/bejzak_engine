#include "vulkan/graphics_context/presentation.h"

#include "common/math/engine_math.h"
#include "common/util/engine_exception.h"
#include "lib/types/util.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context.h"
#include "vulkan/graphics_context/presentation_lib.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/surface/surface.h"
#include "vulkan/wrapper/swapchain/swapchain.h"

namespace vlkn {

Presentation::Presentation(
    std::shared_ptr<Window> window, std::shared_ptr<Instance> instance, Surface&& surface,
    PresentationContext* presentationContext,
    std::unique_ptr<GraphicsContext<false, false>> graphicsContext,
    std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
    const FileLoader& fileLoader)
  : _window(std::move(window)), _instance(std::move(instance)), _surface(std::move(surface)),
    _presentationContext(presentationContext), _graphicsContext(std::move(graphicsContext)),
    _communicationLayer(std::move(communicationLayer)),
    _mouseKeyboardManager(_window->createMouseKeyboardManager()) {}

std::unique_ptr<common::Presentation> Presentation::create(
    std::shared_ptr<Window> window, const FileLoader& fileLoader) {
  std::vector<const char*> requiredExtensions = window->getVulkanExtensions();
#ifdef VALIDATION_LAYERS_ENABLED
  requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif  // VALIDATION_LAYERS_ENABLED
  requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  std::shared_ptr<Instance> instance =
      Instance::createPtr("Bejzak Engine", requiredExtensions, debugCallback1);
#ifdef VALIDATION_LAYERS_ENABLED
  DebugMessenger debugMessenger = DebugMessenger::create(*instance, debugCallback1);
#else
  DebugMessenger debugMessenger;
#endif  // VALIDATION_LAYERS_ENABLED
  Surface surface = Surface::create(*instance, *window);
  std::unique_ptr<PhysicalDevice> physicalDevice =
      PhysicalDevice::create(*instance, surface.getVkSurface());
  std::unique_ptr<LogicalDevice> logicalDevice = LogicalDevice::createPtr(*physicalDevice);

  const auto [width, height] = window->getFramebufferSize();
  Swapchain swapchain =
      SwapchainBuilder()
          .withPreferredPresentMode(VK_PRESENT_MODE_MAILBOX_KHR)
          .build(*logicalDevice, surface.getVkSurface(), VkExtent2D{width, height});

  std::unique_ptr<PresentationContext> presentationContext =
      PresentationContext::create(std::move(swapchain));
  PresentationContext* presentationContextPtr = presentationContext.get();

  std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer =
      engine::PresentationGraphicsCommunication::create();
  auto graphicsContext =
      lib::dynamicUniqueCast<GraphicsContext<false, false>>(GraphicsContext<false, false>::create(
          instance, std::move(debugMessenger), std::move(physicalDevice), std::move(logicalDevice),
          fileLoader, communicationLayer, std::move(presentationContext)));
  return std::unique_ptr<Presentation>(new Presentation(
      std::move(window), std::move(instance), std::move(surface), presentationContextPtr,
      std::move(graphicsContext), std::move(communicationLayer), fileLoader));
}

common::GraphicsContext* Presentation::getGraphicsContext() {
  return _graphicsContext.get();
}

void Presentation::run() {
  if (_graphicsContext == nullptr) {
    throw EngineException("Graphics context must be created before running presentation.");
  }

  std::chrono::steady_clock::time_point previous, current;
  float deltaTime;
  VkResult result;
  // This should be handled outside by engine but for now it is ok here:
  // _mouseKeyboardManager->absorbCursor();
  _mouseKeyboardManager->setKeyboardCallback([&](Keyboard::Key key, int action) {
    switch (key) {
      case Keyboard::Key::Escape:
        _window->close();
        break;
    }
  });
  /////////////////////////
  _graphicsContext->createPresentingResources(_presentationContext->getPresentResources());

  _graphicsContext->initializeResources();
  Camera camera(PerspectiveProjection{glm::radians(45.0f), 1920.0f / 1080.f, 0.01f, 500.0f},
                glm::vec3(0.0f), 5.5f, 0.01f);
  const auto [screenWidth, screenHeight] = _window->getFramebufferSize();
  glm::mat4 tempViewMat(1.0f);
  while (_window->open()) {
    current = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(current - previous).count();
    previous = current;
    _window->pollEvents();
    // This should be handled outside by engine but for now it is ok here:
    camera.updateFromKeyboard(*_mouseKeyboardManager, deltaTime);
    /////////////////////////
    _graphicsContext->waitCompleteExecution();
    _communicationLayer->setCurrentSwapchainImageIndex(_presentationContext->acquireNextImage());
    glm::vec2 mousePos = _mouseKeyboardManager->getMousePosition();
    const glm::vec3 viewDir = common::getWorldSpaceViewDirection(
        mousePos.x, mousePos.y, screenWidth, screenHeight, glm::inverse(tempViewMat),
        glm::inverse(camera.getProjectionMatrix()));
    _communicationLayer->setCameraContexts({
      common::CameraContext{
                            camera.getPosition(),
                            _mouseKeyboardManager->isPressed(Keyboard::Key::R) ? tempViewMat = camera.getViewMatrix() :
                                                             tempViewMat, camera.getProjectionMatrix(), viewDir}
    });
    _communicationLayer->setScreenPos(mousePos.x, mousePos.y);
    _graphicsContext->draw();
    _presentationContext->present();
  }
}

}  // namespace vlkn
