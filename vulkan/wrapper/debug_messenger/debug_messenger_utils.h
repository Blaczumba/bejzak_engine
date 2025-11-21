#pragma once

#include <vulkan/vulkan.h>

VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfoUtility(
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
