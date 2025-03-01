#include "instance.h"

#include "config/config.h"
#include "debug_messenger/debug_messenger_utils.h"
#include "window/window/window.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

Instance::Instance(const VkInstance instance) : _instance(instance) { }

Instance::~Instance() {
    vkDestroyInstance(_instance, nullptr);
}

lib::ErrorOr<std::unique_ptr<Instance>> Instance::create(std::string_view engineName, const std::vector<const char*>& requiredExtensions) {
#ifdef VALIDATION_LAYERS_ENABLED
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
#endif // VALIDATION_LAYERS_ENABLED

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = engineName.data(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = engineName.data(),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

#ifdef VALIDATION_LAYERS_ENABLED
    const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = populateDebugMessengerCreateInfoUtility();
#else
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
#endif // VALIDATION_LAYERS_ENABLED

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    #ifdef VALIDATION_LAYERS_ENABLED
        .pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo,
    #endif // VALIDATION_LAYERS_ENABLED
        .pApplicationInfo = &appInfo,
    #ifdef VALIDATION_LAYERS_ENABLED
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
    #else
        .enabledLayerCount = 0,
    #endif // VALIDATION_LAYERS_ENABLED
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };

    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        return lib::Error("Failed to create instance.");
    }

    return std::unique_ptr<Instance>(new Instance(instance));
}

const VkInstance Instance::getVkInstance() const {
    return _instance;
}

bool Instance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check if all validation layers are in available layers.
    return std::all_of(validationLayers.cbegin(), validationLayers.cend(), [&](const char* layerName) {
        return std::find_if(availableLayers.cbegin(), availableLayers.cend(), [&](const auto& layerProperty) {
            return std::strcmp(layerName, layerProperty.layerName) == 0;
            }) != availableLayers.cend();
    });
}

std::vector<VkPhysicalDevice> Instance::getAvailablePhysicalDevices() const {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    return devices;
}
