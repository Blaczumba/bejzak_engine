#include "debug_messenger_utils.h"

#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "[Vulkan Validation] "
        << "Severity: " << messageSeverity << ", "
        << "Type: " << messageType << std::endl
        << "Message: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

