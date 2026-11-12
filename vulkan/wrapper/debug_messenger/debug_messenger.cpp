#include "debug_messenger.h"

#include <utility>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "debug_messenger_utils.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/util/check.h"

DebugMessenger::DebugMessenger(
    const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger) noexcept
  : _instance(&instance), _debugUtilsMessenger(debugUtilsMessenger) {}

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

DebugMessenger DebugMessenger::create(
    const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  const VkDebugUtilsMessengerCreateInfoEXT createInfo =
      populateDebugMessengerCreateInfoUtility(debugCallback);
  VkDebugUtilsMessengerEXT debugUtilsMessenger;
  if (auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance.getVkInstance(), "vkCreateDebugUtilsMessengerEXT");
      func != nullptr) {
    CHECK_VKCMD(func(instance.getVkInstance(), &createInfo, nullptr, &debugUtilsMessenger),
                "Failed to create VkDebugUtilsMessengerEXT.");
  } else {
    throw EngineException("vkCreateDebugUtilsMessengerEXT was not found.");
  }
  return DebugMessenger(instance, debugUtilsMessenger);
}

std::unique_ptr<DebugMessenger> DebugMessenger::createPtr(
    const Instance& instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
  return std::make_unique<DebugMessenger>(DebugMessenger::create(instance, debugCallback));
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
