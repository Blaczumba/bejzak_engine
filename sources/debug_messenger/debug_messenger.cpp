#include "debug_messenger.h"

#include "debug_messenger_utils.h"
#include "instance/instance.h"

#include <iostream>
#include <stdexcept>

DebugMessenger::DebugMessenger(const Instance& instance) : _instance(instance) {
    const VkDebugUtilsMessengerCreateInfoEXT createInfo = populateDebugMessengerCreateInfoUtility();
    if (CreateDebugUtilsMessengerEXT(&createInfo, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

DebugMessenger::~DebugMessenger() {
    const VkInstance instance = _instance.getVkInstance();
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(_instance.getVkInstance(), _debugMessenger, nullptr);
    }
}

VkResult DebugMessenger::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator) {
    const VkInstance instance = _instance.getVkInstance();
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, &_debugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}