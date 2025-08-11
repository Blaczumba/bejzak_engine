#include "debug_messenger.h"

#include "debug_messenger_utils.h"
#include "vulkan_wrapper/instance/instance.h"
#include "vulkan_wrapper/util/check.h"

DebugMessenger::DebugMessenger(
    const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger)
  : _instance(&instance), _debugUtilsMessenger(debugUtilsMessenger) {}

DebugMessenger::DebugMessenger() : _debugUtilsMessenger(VK_NULL_HANDLE), _instance(nullptr) {}

DebugMessenger::DebugMessenger(DebugMessenger&& debugMessenger) noexcept
  : _debugUtilsMessenger(std::exchange(debugMessenger._debugUtilsMessenger, VK_NULL_HANDLE)),
    _instance(std::exchange(debugMessenger._instance, nullptr)) {}

DebugMessenger& DebugMessenger::operator=(DebugMessenger&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  // TODO what if _debugUtilsMessenger != VK_NULL_HANDLE
  _debugUtilsMessenger = std::exchange(other._debugUtilsMessenger, VK_NULL_HANDLE);
  _instance = std::exchange(other._instance, nullptr);
  return *this;
}

ErrorOr<DebugMessenger> DebugMessenger::create(
    const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo =
      populateDebugMessengerCreateInfoUtility(debugCallback);
  VkDebugUtilsMessengerEXT debugUtilsMessenger;
  if (auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance.getVkInstance(), "vkCreateDebugUtilsMessengerEXT");
      func != nullptr) {
    CHECK_VKCMD(func(instance.getVkInstance(), &createInfo, nullptr, &debugUtilsMessenger));
  } else {
    return Error(VK_ERROR_EXTENSION_NOT_PRESENT);
  }

  return DebugMessenger(instance, debugUtilsMessenger);
}

DebugMessenger::~DebugMessenger() {
  if (_debugUtilsMessenger == VK_NULL_HANDLE) {
    return;
  }
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      _instance->getVkInstance(), "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(_instance->getVkInstance(), _debugUtilsMessenger, nullptr);
  }
}
