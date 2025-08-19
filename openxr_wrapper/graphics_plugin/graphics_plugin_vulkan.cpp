#include "graphics_plugin_vulkan.h"

#include <algorithm>
#include <array>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <span>
#include <unordered_set>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/util/check.h"
#include "vulkan_wrapper/debug_messenger/debug_messenger.h"
#include "vulkan_wrapper/debug_messenger/debug_messenger_utils.h"
#include "vulkan_wrapper/instance/extensions.h"
#include "vulkan_wrapper/util/check.h"

namespace xrw {

GraphicsPluginVulkan::GraphicsPluginVulkan(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback)
  : _debugCallback(debugCallback) {}

std::span<const char* const> GraphicsPluginVulkan::getOpenXrInstanceExtensions() const {
  static constexpr std::array instanceExtensions = {XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};
  return instanceExtensions;
}

const XrBaseInStructure* GraphicsPluginVulkan::getGraphicsBinding() const {
  return nullptr;
}

XrSwapchainImageBaseHeader* GraphicsPluginVulkan::allocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo& swapchain_create_info) {
  return nullptr;
}

ErrorOr<int64_t> GraphicsPluginVulkan::selectSwapchainFormat(
    std::span<const int64_t> runtimeFormats) const {
  static constexpr VkFormat preferredFromats[] = {
    VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM};

  if (auto it = std::find_first_of(std::cbegin(preferredFromats), std::cend(preferredFromats),
                                   std::cbegin(runtimeFormats), std::cend(runtimeFormats));
      it != std::cend(preferredFromats)) {
    return *it;
  }
  return Error(EngineError::NOT_FOUND);
}

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

ErrorOr<lib::Buffer<VkExtensionProperties>> GetAvailableInstanceExtensions(
    std::string_view layerName) {
  uint32_t count;
  CHECK_VKCMD(vkEnumerateInstanceExtensionProperties(layerName.data(), &count, nullptr));
  lib::Buffer<VkExtensionProperties> available_extensions(count);
  CHECK_VKCMD(vkEnumerateInstanceExtensionProperties(
      layerName.data(), &count, available_extensions.data()));
  return available_extensions;
}

ErrorOr<Instance> createInstance(
    std::string_view engineName, std::span<const char* const> requiredExtensions,
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback, XrInstance xrInstance,
    XrSystemId systemId) {
#ifdef VALIDATION_LAYERS_ENABLED
  if (!checkValidationLayerSupport()) {
    return Error(VK_ERROR_FEATURE_NOT_PRESENT);
  }
#endif  // VALIDATION_LAYERS_ENABLED

  XrGraphicsRequirementsVulkan2KHR graphics_requirements = {
    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
  PFN_xrGetVulkanGraphicsRequirements2KHR pfn_get_vulkan_graphics_requirements_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
      xrInstance, "xrGetVulkanGraphicsRequirements2KHR",
      reinterpret_cast<PFN_xrVoidFunction*>(&pfn_get_vulkan_graphics_requirements_khr)));
  if (pfn_get_vulkan_graphics_requirements_khr == nullptr) {
    return Error(EngineError::NOT_FOUND);
  }
  CHECK_XRCMD(
      pfn_get_vulkan_graphics_requirements_khr(xrInstance, systemId, &graphics_requirements));
  PFN_xrCreateVulkanInstanceKHR pfn_xr_create_vulkan_instance_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
      xrInstance, "xrCreateVulkanInstanceKHR",
      reinterpret_cast<PFN_xrVoidFunction*>(&pfn_xr_create_vulkan_instance_khr)));
  if (pfn_xr_create_vulkan_instance_khr == nullptr) {
    return Error(EngineError::NOT_FOUND);
  }
  const VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VrApp",
    .pEngineName = "BejzakEngine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0};

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
      xrInstance, &vkInstanceCreateInfoKhr, &vkInstance, &instanceCreateResult));
  CHECK_VKCMD(instanceCreateResult);
  return Instance::wrap(vkInstance);
}

ErrorOr<std::unique_ptr<PhysicalDevice>> createPhysicalDevice(
    XrInstance xrInstance, XrSystemId systemId, Instance& instance) {
  PFN_xrGetVulkanGraphicsDevice2KHR pfn_get_vulkan_graphics_device_khr = nullptr;
  CHECK_XRCMD(xrGetInstanceProcAddr(
      xrInstance, "xrGetVulkanGraphicsDevice2KHR",
      reinterpret_cast<PFN_xrVoidFunction*>(&pfn_get_vulkan_graphics_device_khr)));

  if (pfn_get_vulkan_graphics_device_khr == nullptr) {
    return Error(EngineError::NOT_FOUND);
  }

  const XrVulkanGraphicsDeviceGetInfoKHR vulkan_graphics_device_get_info_khr = {
    .type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR,
    .systemId = systemId,
    .vulkanInstance = instance.getVkInstance()};

  VkPhysicalDevice physicalDevice;
  CHECK_XRCMD(pfn_get_vulkan_graphics_device_khr(
      xrInstance, &vulkan_graphics_device_get_info_khr, &physicalDevice));
  return PhysicalDevice::wrap(physicalDevice, instance);
}

}  // namespace

Status GraphicsPluginVulkan::initialize(XrInstance xrInstance, XrSystemId systemId) {
  static constexpr const char* extensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
  ASSIGN_OR_RETURN(_instance, createInstance("VR BejzakEngine", extensions, _debugCallback,
                                             xrInstance, systemId));
#ifdef VALIDATION_LAYERS_ENABLED
  ASSIGN_OR_RETURN(_debugMessenger, DebugMessenger::create(_instance, _debugCallback));
#endif
  ASSIGN_OR_RETURN(_physicalDevice, createPhysicalDevice(xrInstance, systemId, _instance));
  return StatusOk();
}

}  // namespace xrw
