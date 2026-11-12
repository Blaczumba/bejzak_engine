#include "vulkan/graphics_context/presentation.h"

#include "engine/system.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context.h"
#include "vulkan/graphics_context/presentation_lib.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/instance/validation_layers.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/physical_device/extensions_connector.h"
#include "vulkan/wrapper/physical_device/optional_extended_features.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/surface/surface.h"
#include "vulkan/wrapper/swapchain/swapchain.h"

namespace vlkn {

Presentation::Presentation(
    engine::System& system, std::shared_ptr<Window> window, std::shared_ptr<Instance> instance,
    std::shared_ptr<DebugMessenger> debugMessenger, Surface&& surface,
    std::shared_ptr<PhysicalDevice> physicalDevice,
    PhysicalDeviceOptionalExtendedFeatures optionalExtendedFeatures,
    std::shared_ptr<LogicalDevice> logicalDevice,
    std::shared_ptr<PresentationContext> presentationContext,
    std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer)
  : _system(system), _window(std::move(window)), _instance(std::move(instance)),
    _debugMessenger(std::move(debugMessenger)), _surface(std::move(surface)),
    _physicalDevice(std::move(physicalDevice)), _optionalExtendedFeatures(optionalExtendedFeatures),
    _logicalDevice(std::move(logicalDevice)), _presentationContext(presentationContext),
    _communicationLayer(std::move(communicationLayer)) {}

std::unique_ptr<common::Presentation> Presentation::create(
    engine::System& system, std::shared_ptr<Window> window) {
  std::vector<const char*> requiredExtensions = window->getVulkanExtensions();
#ifdef VALIDATION_LAYERS_ENABLED
  requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif  // VALIDATION_LAYERS_ENABLED
  requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  std::shared_ptr<Instance> instance =
      Instance::createPtr("Bejzak Engine", requiredExtensions, debugCallback1);
  std::shared_ptr<DebugMessenger> debugMessenger;
#ifdef VALIDATION_LAYERS_ENABLED
  debugMessenger = DebugMessenger::createPtr(*instance, debugCallback1);
#endif
  Surface surface = Surface::create(*instance, *window);
  std::shared_ptr<PhysicalDevice> physicalDevice =
      PhysicalDevice::create(*instance, surface.getVkSurface());

  std::unique_ptr<ExtensionsConnector> extensionsConnector =
      std::make_unique<ExtensionsConnector>(*physicalDevice);
  extensionsConnector->withDescriptorIndexingExtension()
      .withBufferDeviceAddressExtension()
      .withIndexTypeUint8Extension()
      .withInheritedViewportScissorExtension()  // This should be the optional extension.
      .withMultiviewExtension()
      .withStorage8BitExtension()
      .withStorage16BitExtension()
      .withSynchronization2();
  PhysicalDeviceOptionalExtendedFeatures extendedFeatures;
  const char* fragmentShadingRateAttachmentExtension;
  std::tie(extendedFeatures.fragmentShadingRateModifier, fragmentShadingRateAttachmentExtension) =
      AttachmentBasedFragmentShadingRateModifier::create(
          *physicalDevice, *extensionsConnector,
          AttachmentBasedFragmentShadingRateModifier::SupportedFeature::FRAGMENT_SHADING_RATE);

  std::shared_ptr<LogicalDevice> logicalDevice =
      LogicalDeviceBuilder()
          .withPhysicalDeviceFeatures2(extensionsConnector->getVkPhysicalDeviceFeatures2())
          .withExtensions(extensionsConnector->getAvailableRequestedDeviceExtensions())
#ifdef VALIDATION_LAYERS_ENABLED
          .withValidationLayers(validationLayers)
#endif  // VALIDATION_LAYERS_ENABLED
          .buildPtr(*physicalDevice);

  const auto [width, height] = window->getFramebufferSize();
  Swapchain swapchain =
      SwapchainBuilder()
          .withPreferredPresentMode(VK_PRESENT_MODE_MAILBOX_KHR)
          .build(*logicalDevice, surface.getVkSurface(), VkExtent2D{width, height});

  std::shared_ptr<PresentationContext> presentationContext =
      PresentationContext::create(std::move(swapchain));

  std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer =
      engine::PresentationGraphicsCommunication::create(1);

  return std::unique_ptr<Presentation>(new Presentation(
      system, std::move(window), std::move(instance), std::move(debugMessenger), std::move(surface),
      std::move(physicalDevice), extendedFeatures, std::move(logicalDevice),
      std::move(presentationContext), std::move(communicationLayer)));
}

std::unique_ptr<common::GraphicsContext> Presentation::createGraphicsContext() {
  assert(_graphicsContextCreated == false && "Graphics Context can be created once at maximum.");
  _graphicsContextCreated = true;

  auto graphicsContext = GraphicsContext<false>::create(
      _instance, _debugMessenger, _physicalDevice, _logicalDevice, _optionalExtendedFeatures,
      _system.getFileLoader(), _communicationLayer, _presentationContext);
  graphicsContext->createPresentingResources(_presentationContext->getPresentResources());
  graphicsContext->initializeResources();
  return graphicsContext;
}

engine::PresentationGraphicsCommunication&
Presentation::getPresentationGraphicsCommunicationLayer() {
  return *_communicationLayer;
}

bool Presentation::shouldClose() {
  return !_window->open();
}

bool Presentation::pollActionsAndEvents() {
  _window->pollEvents();
  return true;
}

bool Presentation::beginFrame() {
  _communicationLayer->setCurrentSwapchainImageIndex(_presentationContext->acquireNextImage());
  return true;
}

void Presentation::endFrame() {
  _presentationContext->present();
}

}  // namespace vlkn
