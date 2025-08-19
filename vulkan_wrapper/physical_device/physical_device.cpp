#include "physical_device.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "lib/algorithm.h"
#include "vulkan_wrapper/instance/extensions.h"
#include "vulkan_wrapper/logical_device/logical_device.h"

namespace {

std::unordered_set<std::string_view> checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  lib::Buffer<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, availableExtensions.data());

  std::unordered_set<std::string_view> availableExtensionNames;
  availableExtensionNames.reserve(extensionCount);
  for (const VkExtensionProperties& ext : availableExtensions) {
    availableExtensionNames.emplace(ext.extensionName);
  }

  std::unordered_set<std::string_view> supportedRequestedExtensions;
  for (const char* requested : requestedDeviceExtensions) {
    if (availableExtensionNames.contains(requested)) {
      supportedRequestedExtensions.emplace(requested);
    }
  }

  return supportedRequestedExtensions;
}

bool areQueueFamilyIndicesComplete(const QueueFamilyIndices& indices) {
  return indices.graphicsFamily.has_value() && indices.presentFamily.has_value()
         && indices.computeFamily.has_value() && indices.transferFamily.has_value();
}

lib::Buffer<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  lib::Buffer<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
  return queueFamilies;
}

QueueFamilyIndices findQueueFamilyIncides(VkPhysicalDevice device, VkSurfaceKHR surface) {
  lib::Buffer<VkQueueFamilyProperties> queueFamilies = getQueueFamilyProperties(device);
  QueueFamilyIndices indices;

  for (uint32_t i = 0; i < queueFamilies.size() && !areQueueFamilyIndicesComplete(indices); ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      indices.computeFamily = i;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      indices.transferFamily = i;
    }

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
  }
  return indices;
}

QueueFamilyIndices findQueueFamilyIncides(VkPhysicalDevice device) {
  lib::Buffer<VkQueueFamilyProperties> queueFamilies = getQueueFamilyProperties(device);
  QueueFamilyIndices indices;

  for (uint32_t i = 0; i < queueFamilies.size() && !areQueueFamilyIndicesComplete(indices); ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = indices.presentFamily = i;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      indices.computeFamily = i;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      indices.transferFamily = i;
    }
  }
  return indices;
}

SwapChainSupportDetails querySwapchainSupportDetails(
    VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats = lib::Buffer<VkSurfaceFormatKHR>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
  if (presentModeCount != 0) {
    details.presentModes = lib::Buffer<VkPresentModeKHR>(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

}  // namespace

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice, const Instance& instance,
                               const QueueFamilyIndices& queueFamilyIndices,
                               std::optional<SwapChainSupportDetails> swapchainSupportDetails)
  : _device(physicalDevice), _instance(instance),
    _availableRequestedExtensions(checkDeviceExtensionSupport(physicalDevice)),
    _queueFamilyIndices(queueFamilyIndices), _swapchainSupportDetails(swapchainSupportDetails) {
  vkGetPhysicalDeviceProperties(_device, &_properties);
}

ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::create(
    const Instance& instance, VkSurfaceKHR surface) {
  ASSIGN_OR_RETURN(
      const lib::Buffer<VkPhysicalDevice> devices, instance.getAvailablePhysicalDevices());
  for (const auto device : devices) {
    const QueueFamilyIndices queueFamilyIndices = findQueueFamilyIncides(device, surface);
    const SwapChainSupportDetails swapchainSupportDetails =
        querySwapchainSupportDetails(device, surface);

    const bool swapChainAdequate =
        !swapchainSupportDetails.formats.empty() && !swapchainSupportDetails.presentModes.empty();

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    const bool discreteGPU = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    if (lib::cont_all_of(
            std::initializer_list{
              areQueueFamilyIndicesComplete(queueFamilyIndices), swapChainAdequate,
              static_cast<bool>(supportedFeatures.samplerAnisotropy)},
            [](bool condition) {
              return condition;
            })) {
      return std::unique_ptr<PhysicalDevice>(
          new PhysicalDevice(device, instance, queueFamilyIndices, swapchainSupportDetails));
    }
  }
  return Error(EngineError::NOT_FOUND);
}

ErrorOr<std::unique_ptr<PhysicalDevice>> PhysicalDevice::wrap(
    VkPhysicalDevice physicalDevice, const Instance& instance) {
  if (physicalDevice == VK_NULL_HANDLE) {
    return Error(EngineError::NULLPTR_REFERENCE);
  }
  return std::unique_ptr<PhysicalDevice>(
      new PhysicalDevice(physicalDevice, instance, findQueueFamilyIncides(physicalDevice)));
}

VkPhysicalDevice PhysicalDevice::getVkPhysicalDevice() const {
  return _device;
}

const Instance& PhysicalDevice::getInstance() const {
  return _instance;
}

bool PhysicalDevice::hasAvailableExtension(std::string_view extension) const {
  return _availableRequestedExtensions.contains(extension);
}

float PhysicalDevice::getMaxSamplerAnisotropy() const {
  return _properties.limits.maxSamplerAnisotropy;
}

size_t PhysicalDevice::getMemoryAlignment(size_t size) const {
  const size_t minUboAlignment = _properties.limits.minUniformBufferOffsetAlignment;
  return minUboAlignment > 0 ? (size + minUboAlignment - 1) & ~(minUboAlignment - 1) : size;
}

lib::Buffer<const char*> PhysicalDevice::getAvailableExtensions() const {
  lib::Buffer<const char*> extensions(_availableRequestedExtensions.size());
  std::transform(_availableRequestedExtensions.cbegin(), _availableRequestedExtensions.cend(),
                 extensions.begin(), [](std::string_view extension) {
                   return extension.data();
                 });
  return extensions;
}

const QueueFamilyIndices& PhysicalDevice::getQueueFamilyIndices() const {
  return _queueFamilyIndices;
}

const std::optional<SwapChainSupportDetails>& PhysicalDevice::getSwapchainSupportDetails() const {
  return _swapchainSupportDetails;
}
