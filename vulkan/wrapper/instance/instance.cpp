#include "instance.h"

#include <string_view>
#include <unordered_set>

#include "common/util/engine_exception.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger_utils.h"
#include "vulkan/wrapper/instance/extensions.h"
#include "vulkan/wrapper/util/check.h"

Instance::Instance(VkInstance instance) noexcept : _instance(instance) {}

Instance::Instance(Instance&& instance) noexcept
  : _instance(std::exchange(instance._instance, VK_NULL_HANDLE)) {}

Instance& Instance::operator=(Instance&& instance) noexcept {
  if (this == &instance) {
    return *this;
  }
  // TODO what if _instance != VK_NULL_HANDLE
  _instance = std::exchange(instance._instance, VK_NULL_HANDLE);
  return *this;
}

Instance::~Instance() {
  if (_instance != VK_NULL_HANDLE) {
    vkDestroyInstance(_instance, nullptr);
  }
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

VkInstance createVkInstance(
    std::string_view engineName, std::span<const char* const> requiredExtensions,
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
#ifdef VALIDATION_LAYERS_ENABLED
  if (!checkValidationLayerSupport()) {
    throw EngineException("Validation layers not supported.");
  }
#endif  // VALIDATION_LAYERS_ENABLED

  const VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = engineName.data(),
    .applicationVersion = VK_MAKE_VERSION(1, 3, 0),
    .pEngineName = engineName.data(),
    .engineVersion = VK_MAKE_VERSION(1, 3, 0),
    .apiVersion = VK_API_VERSION_1_3};

#ifdef VALIDATION_LAYERS_ENABLED
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo =
      populateDebugMessengerCreateInfoUtility(debugCallback);
#endif  // VALIDATION_LAYERS_ENABLED

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

  VkInstance instance;
  CHECK_VKCMD(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create VkInstance.");
  return instance;
}

}  // namespace

Instance Instance::create(
    std::string_view engineName, std::span<const char* const> requiredExtensions,
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  return Instance(createVkInstance(engineName, requiredExtensions, debugCallback));
}

std::unique_ptr<Instance> Instance::createPtr(
    std::string_view engineName, std::span<const char* const> requiredExtensions,
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  return std::unique_ptr<Instance>(
      new Instance(createVkInstance(engineName, requiredExtensions, debugCallback)));
}

Instance Instance::wrap(VkInstance instance) {
  if (instance == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around Instance.");
  }

  return Instance(instance);
}

std::unique_ptr<Instance> Instance::wrapPtr(VkInstance instance) {
  if (instance == VK_NULL_HANDLE) {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around Instance.");
  }

  return std::unique_ptr<Instance>(new Instance(instance));
}

VkInstance Instance::getVkInstance() const noexcept {
  return _instance;
}

lib::Buffer<VkPhysicalDevice> Instance::getAvailablePhysicalDevices() const {
  uint32_t deviceCount;
  vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw EngineException("No physical devices found.");
  }

  lib::Buffer<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

  return devices;
}
