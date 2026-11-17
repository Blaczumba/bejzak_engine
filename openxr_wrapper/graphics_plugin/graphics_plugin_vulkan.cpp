#include "graphics_plugin_vulkan.h"

#include <algorithm>
#include <format>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "common/format.h"
#include "vulkan/lib/format.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/util/check.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger_utils.h"
#include "vulkan/wrapper/instance/validation_layers.h"
#include "vulkan/wrapper/logical_device/lib.h"
#include "vulkan/wrapper/physical_device/extensions_connector.h"
#include "vulkan/wrapper/physical_device/optional_extended_features.h"
#include "vulkan/wrapper/util/check.h"

namespace xrw {
namespace {

bool checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  lib::Buffer<VkLayerProperties> availableLayers(layerCount);
  if (layerCount > 0) {
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
  }

  std::unordered_set<std::string_view> availableLayerNames;
  availableLayerNames.reserve(layerCount);
  for (const VkLayerProperties& layer : availableLayers) {
    availableLayerNames.emplace(layer.layerName);
  }

  for (const char* requested : validationLayers) {
    if (!availableLayerNames.contains(requested)) {
      return false;
    }
  }
  return true;
}

lib::Buffer<VkExtensionProperties> GetAvailableInstanceExtensions(std::string_view layerName) {
  uint32_t count;
  CHECK_VKCMD(vkEnumerateInstanceExtensionProperties(layerName.data(), &count, nullptr),
              "Failed to vkEnumerateInstanceExtensionProperties.");
  lib::Buffer<VkExtensionProperties> available_extensions(count);
  CHECK_VKCMD(
      vkEnumerateInstanceExtensionProperties(layerName.data(), &count, available_extensions.data()),
      "Failed to vkEnumerateInstanceExtensionProperties.");
  return available_extensions;
}

std::unique_ptr<Instance> createInstance(
    std::string_view engineName, std::span<const char* const> requiredExtensions,
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback, XrInstance xrInstance,
    XrSystemId systemId) {
#ifdef VALIDATION_LAYERS_ENABLED
  if (!checkValidationLayerSupport()) [[unlikely]] {
    throw EngineException("Validation layers not supported.");
  }
#endif  // VALIDATION_LAYERS_ENABLED

  XrGraphicsRequirementsVulkan2KHR graphics_requirements = {
    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
  PFN_xrGetVulkanGraphicsRequirements2KHR pfn_get_vulkan_graphics_requirements_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
                  xrInstance, "xrGetVulkanGraphicsRequirements2KHR",
                  reinterpret_cast<PFN_xrVoidFunction*>(&pfn_get_vulkan_graphics_requirements_khr)),
              "Failed to xrGetVulkanGraphicsRequirements2KHR.");
  if (pfn_get_vulkan_graphics_requirements_khr == nullptr) [[unlikely]] {
    throw EngineException(
        "Failed to find xrGetVulkanGraphicsRequirements2KHR with xrGetInstanceProcAddr.");
  }
  CHECK_XRCMD(
      pfn_get_vulkan_graphics_requirements_khr(xrInstance, systemId, &graphics_requirements),
      "Failed to pfn_get_vulkan_graphics_requirements_khr.");
  PFN_xrCreateVulkanInstanceKHR pfn_xr_create_vulkan_instance_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
                  xrInstance, "xrCreateVulkanInstanceKHR",
                  reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xr_create_vulkan_instance_khr)),
              "Failed to xrGetInstanceProcAddr.");
  if (pfn_xr_create_vulkan_instance_khr == nullptr) [[unlikely]] {
    throw EngineException("Failed to find xrCreateVulkanInstanceKHR with xrGetInstanceProcAddr.");
  }
  const VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VrApp",
    .pEngineName = "BejzakEngine",
    .engineVersion = VK_MAKE_VERSION(1, 3, 0),
    .apiVersion = VK_API_VERSION_1_3};

#ifdef VALIDATION_LAYERS_ENABLED
  const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo =
      populateDebugMessengerCreateInfoUtility(debugCallback);
#endif

  const VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef VALIDATION_LAYERS_ENABLED
    .pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo,
#endif  // VALIDATION_LAYERS_ENABLED
    .pApplicationInfo = &appInfo,
#ifdef VALIDATION_LAYERS_ENABLED
    .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
    .ppEnabledLayerNames = validationLayers.data(),
#else
    .enabledLayerCount = 0,
#endif  // VALIDATION_LAYERS_ENABLED
    .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
    .ppEnabledExtensionNames = requiredExtensions.data()};

  const XrVulkanInstanceCreateInfoKHR vkInstanceCreateInfoKhr = {
    .type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR,
    .systemId = systemId,
    .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
    .vulkanCreateInfo = &createInfo,
  };

  VkResult instanceCreateResult;
  VkInstance vkInstance;
  CHECK_XRCMD(pfn_xr_create_vulkan_instance_khr(
                  xrInstance, &vkInstanceCreateInfoKhr, &vkInstance, &instanceCreateResult),
              "Failed to xrCreateVulkanInstanceKHR.");
  CHECK_VKCMD(instanceCreateResult, "Failed to crete VkInstance.");
  return Instance::wrapPtr(vkInstance);
}

std::unique_ptr<PhysicalDevice> createPhysicalDevice(
    XrInstance xrInstance, XrSystemId systemId, Instance& instance) {
  PFN_xrGetVulkanGraphicsDevice2KHR pfn_get_vulkan_graphics_device_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
                  xrInstance, "xrGetVulkanGraphicsDevice2KHR",
                  reinterpret_cast<PFN_xrVoidFunction*>(&pfn_get_vulkan_graphics_device_khr)),
              "Failed to xrGetInstanceProcAddr.");

  if (pfn_get_vulkan_graphics_device_khr == nullptr) [[unlikely]] {
    throw EngineException(
        "Failed to find xrGetVulkanGraphicsDevice2KHR with xrGetInstanceProcAddr.");
  }

  const XrVulkanGraphicsDeviceGetInfoKHR vulkan_graphics_device_get_info_khr = {
    .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
    .systemId = systemId,
    .vulkanInstance = instance.getVkInstance()};

  VkPhysicalDevice physicalDevice;
  CHECK_XRCMD(pfn_get_vulkan_graphics_device_khr(
                  xrInstance, &vulkan_graphics_device_get_info_khr, &physicalDevice),
              "Failed to xrGetVulkanGraphicsDevice2KHR.");
  return PhysicalDevice::wrap(physicalDevice, instance);
}

std::unique_ptr<LogicalDevice> createLogicalDevice(
    XrInstance xrInstance, XrSystemId systemId, const PhysicalDevice& physicalDevice,
    const VkPhysicalDeviceFeatures2& physicalDeviceFeatures, std::span<const char*> extensions) {
  const std::vector<VkDeviceQueueCreateInfo> queueCreateInfos =
      getDeviceQueueCreateInfos(physicalDevice.getQueueFamilyIndices());
  const VkDeviceCreateInfo deviceCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &physicalDeviceFeatures,
    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
#ifdef VALIDATION_LAYERS_ENABLED
    .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
    .ppEnabledLayerNames = validationLayers.data(),
#endif  // VALIDATION_LAYERS_ENABLED
    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data()};

  const XrVulkanDeviceCreateInfoKHR vulkan_device_create_info_khr = {
    .type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR,
    .systemId = systemId,
    .pfnGetInstanceProcAddr = &vkGetInstanceProcAddr,
    .vulkanPhysicalDevice = physicalDevice.getVkPhysicalDevice(),
    .vulkanCreateInfo = &deviceCreateInfo};

  PFN_xrCreateVulkanDeviceKHR pfnXrCreateVulkanDeviceKhr = nullptr;
  CHECK_XRCMD(
      xrGetInstanceProcAddr(xrInstance, "xrCreateVulkanDeviceKHR",
                            reinterpret_cast<PFN_xrVoidFunction*>(&pfnXrCreateVulkanDeviceKhr)),
      "Failed to xrGetInstanceProcAddr.");
  if (pfnXrCreateVulkanDeviceKhr == nullptr) [[unlikely]] {
    throw EngineException("Failed to find xrCreateVulkanDeviceKHR with xrGetInstanceProcAddr.");
  }

  VkResult vulkanDeviceCreateResult = VK_SUCCESS;
  VkDevice logicalDevice;
  CHECK_XRCMD(pfnXrCreateVulkanDeviceKhr(xrInstance, &vulkan_device_create_info_khr, &logicalDevice,
                                         &vulkanDeviceCreateResult),
              "Failed to xrCreateVulkanDeviceKHR.");
  CHECK_VKCMD(vulkanDeviceCreateResult, "Failed to create VkDevice.");
  return LogicalDevice::wrapPtr(logicalDevice, physicalDevice);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  spdlog::warn("[Vulkan Validation] Severity: {}, Type: {}, Message: {}.",
               (uint32_t)messageSeverity, (uint32_t)messageType, pCallbackData->pMessage);

  return VK_FALSE;
}

lib::Buffer<int64_t> enumerateSwapchainFormats(XrSession session) {
  uint32_t formatCount = 0;
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr),
              "Failed to xrEnumerateSwapchainFormats.");
  lib::Buffer<int64_t> swapchainFormats(formatCount);
  CHECK_XRCMD(xrEnumerateSwapchainFormats(
      session, static_cast<uint32_t>(swapchainFormats.size()),
      &formatCount, swapchainFormats.data()),
              "Failed to xrEnumerateSwapchainFormats.");
  return swapchainFormats;
}

}  // namespace

GraphicsPluginVulkan::GraphicsPluginVulkan(
    std::shared_ptr<Instance> instance, std::shared_ptr<DebugMessenger> debugMessenger,
    std::shared_ptr<PhysicalDevice> physicalDevice,
    PhysicalDeviceOptionalExtendedFeatures extendedFeatures,
    std::shared_ptr<LogicalDevice> logicalDevice) noexcept
  : _instance(std::move(instance)), _debugMessenger(std::move(debugMessenger)),
    _physicalDevice(std::move(physicalDevice)), _extendedFeatures(extendedFeatures),
    _logicalDevice(std::move(logicalDevice)) {
  _graphicsBinding = XrGraphicsBindingVulkanKHR{
    .type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR,
    .instance = _instance->getVkInstance(),
    .physicalDevice = _physicalDevice->getVkPhysicalDevice(),
    .device = _logicalDevice->getVkDevice(),
    .queueFamilyIndex = _physicalDevice->getQueueFamilyIndices().universalFamily};
}

std::unique_ptr<GraphicsPlugin> GraphicsPluginVulkan::create(
    XrInstance xrInstance, XrSystemId xrSystemId) {
  static constexpr const char* extensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  std::shared_ptr<Instance> instance =
      createInstance("VR BejzakEngine", extensions, debugCallback, xrInstance, xrSystemId);
  std::shared_ptr<DebugMessenger> debugMessenger;
#ifdef VALIDATION_LAYERS_ENABLED
  debugMessenger = DebugMessenger::createPtr(*instance, debugCallback);
#endif
  std::unique_ptr<PhysicalDevice> physicalDevice =
      createPhysicalDevice(xrInstance, xrSystemId, *instance);
  ExtensionsConnector extensionsConnector(*physicalDevice);
  extensionsConnector.withDescriptorIndexingExtension()
      .withBufferDeviceAddressExtension()
      .withIndexTypeUint8Extension()
      .withMultiviewExtension()
      .withSynchronization2();
  PhysicalDeviceOptionalExtendedFeatures extendedFeatures;
  const char* fragmentShadingRateAttachmentExtension;
  std::tie(extendedFeatures.fragmentShadingRateModifier, fragmentShadingRateAttachmentExtension) =
      AttachmentBasedFragmentShadingRateModifier::create(
          *physicalDevice, extensionsConnector,
          AttachmentBasedFragmentShadingRateModifier::SupportedFeature::
              FRAGMENT_DENSITY_MAP_OFFSET);

  std::unique_ptr<LogicalDevice> logicalDevice = createLogicalDevice(
      xrInstance, xrSystemId, *physicalDevice, extensionsConnector.getVkPhysicalDeviceFeatures2(),
      extensionsConnector.getAvailableRequestedDeviceExtensions());
  return std::unique_ptr<GraphicsPluginVulkan>(new GraphicsPluginVulkan(
      std::move(instance), std::move(debugMessenger), std::move(physicalDevice), extendedFeatures,
      std::move(logicalDevice)));
}

GraphicsPluginVulkan::~GraphicsPluginVulkan() {
  for (const auto& [swapchain, views] : _swapchainViews) {
    for (VkImageView view : views) {
      vkDestroyImageView(_logicalDevice->getVkDevice(), view, nullptr);
    }
  }
}

const XrBaseInStructure* GraphicsPluginVulkan::getGraphicsBinding() const {
  return reinterpret_cast<const XrBaseInStructure*>(&_graphicsBinding);
}

std::optional<int64_t> GraphicsPluginVulkan::selectFirstSupportedSwapchainFormat(XrSession session,
                                                                                 std::initializer_list<common::Format> formats) const {
  lib::Buffer<int64_t> swapchainFormats = enumerateSwapchainFormats(session);
  for (common::Format format : formats) {
    const int64_t translatedFormat = vlkn::translateFormat(format);
    for (int64_t supportedFormat : swapchainFormats) {
      if (translatedFormat == supportedFormat) {
        return supportedFormat;
      }
    }
  }
  return std::nullopt;
}

std::optional<int64_t> GraphicsPluginVulkan::isSupportedSwapchainFormat(XrSession session, common::Format format) const {
  lib::Buffer<int64_t> swapchainFormats = enumerateSwapchainFormats(session);
  auto it = std::find(swapchainFormats.cbegin(), swapchainFormats.cend(), vlkn::translateFormat(format));
  if (it != swapchainFormats.cend()) {
    return *it;
  }
  return std::nullopt;
}

void GraphicsPluginVulkan::createSwapchainContext(
    XrSwapchain swapchain, int64_t format, uint32_t width, uint32_t height, uint32_t layerCount) {
  uint32_t imageCount;
  CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr),
              "Failed to xrEnumerateSwapchainImages.");
  lib::Buffer<XrSwapchainImageVulkanKHR> images(
      imageCount, {.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
  CHECK_XRCMD(
      xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount,
                                 reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),
      "Failed to xrEnumerateSwapchainImages.");

  lib::Buffer<VkImageView>& imageViews = _swapchainViews[swapchain] =
      lib::Buffer<VkImageView>(imageCount);
  std::transform(
      images.cbegin(), images.cend(), imageViews.begin(),
      [&](const XrSwapchainImageVulkanKHR& image) {
        const VkImageViewCreateInfo imageViewCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = image.image,
          .viewType = (layerCount >= 2) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
          .format = static_cast<VkFormat>(format),
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                               .baseMipLevel = 0,
                               .levelCount = 1,
                               .baseArrayLayer = 0,
                               .layerCount = layerCount}
        };
        return _logicalDevice->createImageView(imageViewCreateInfo);
      });

  _presentResources[swapchain] = common::PresentResources{
    .imageFormat = format,
    .width = width,
    .height = height,
    .numLayers = layerCount,
    .imageViews = imageViews.data(),
    .imageViewsCount = static_cast<uint32_t>(imageViews.size()),
    .multiview = (layerCount == 2)};
}

common::PresentResources GraphicsPluginVulkan::getSwapchainContext(XrSwapchain swapchain) {
  auto it = _presentResources.find(swapchain);
  if (it == _presentResources.cend()) {
    throw EngineException(std::format("XrSwapchain {} does not exist.", (size_t)swapchain));
  }

  return it->second;
}

void GraphicsPluginVulkan::initialize(XrInstance xrInstance, XrSystemId systemId) {}

void GraphicsPluginVulkan::createResources() {}

std::unique_ptr<common::GraphicsContext> GraphicsPluginVulkan::createGraphicsContext(
    std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
    const FileLoader& fileLoader) {
  return vlkn::GraphicsContext<true>::create(
      _instance, _debugMessenger, _physicalDevice, _logicalDevice, _extendedFeatures, fileLoader,
      std::move(communicationLayer), nullptr);
}

}  // namespace xrw
