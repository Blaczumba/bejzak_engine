#include "debug_messenger.h"

#include "debug_messenger_utils.h"
#include "instance/instance.h"

#include <iostream>
#include <stdexcept>

DebugMessenger::DebugMessenger(const Instance& instance, VkDebugUtilsMessengerEXT debugUtilsMessenger) : _instance(instance), _debugUtilsMessenger(debugUtilsMessenger) {}

ErrorOr<std::unique_ptr<DebugMessenger>> DebugMessenger::create(const Instance& instance) {
    static constexpr VkDebugUtilsMessengerCreateInfoEXT createInfo = populateDebugMessengerCreateInfoUtility();
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    if (auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance.getVkInstance(), "vkCreateDebugUtilsMessengerEXT"); func != nullptr) {
        if (VkResult result = func(instance.getVkInstance(), &createInfo, nullptr, &debugUtilsMessenger); result != VK_SUCCESS) {
            return Error(result);
        }
    }
    else {
        return Error(VK_ERROR_EXTENSION_NOT_PRESENT);
    }

    return std::unique_ptr<DebugMessenger>(new DebugMessenger(instance, debugUtilsMessenger));
}

DebugMessenger::~DebugMessenger() {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance.getVkInstance(), "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(_instance.getVkInstance(), _debugUtilsMessenger, nullptr);
    }
}
