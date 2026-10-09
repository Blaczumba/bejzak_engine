#include "physical_device.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/physical_device/optional_extended_features.h"

namespace {

std::unordered_set<std::string> checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  lib::Buffer<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, availableExtensions.data());

  std::unordered_set<std::string> availableExtensionNames;
  availableExtensionNames.reserve(extensionCount);
  std::transform(availableExtensions.cbegin(), availableExtensions.cend(),
                 std::inserter(availableExtensionNames, availableExtensionNames.begin()),
                 [](const VkExtensionProperties& properties) {
                   return properties.extensionName;
                 });
  return availableExtensionNames;
}

lib::Buffer<VkQueueFamilyProperties> getQueueFamilyProperties(VkPhysicalDevice device) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  lib::Buffer<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
  return queueFamilies;
}

QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) {
  lib::Buffer<VkQueueFamilyProperties> queueFamilies = getQueueFamilyProperties(device);
  std::optional<uint32_t> universalFamily;
  std::optional<uint32_t> dedicatedPresentFamily;
  std::optional<uint32_t> dedicatedTransferFamily;
  std::optional<uint32_t> dedicatedComputeFamily;

  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
    const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
    const VkQueueFlags flags = queueFamily.queueFlags;

    if (!universalFamily.has_value()) {
      if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
        universalFamily = i;
      }
    }

    if (!dedicatedComputeFamily.has_value()) {
      if ((flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT)) {
        dedicatedComputeFamily = i;
      }
    }

    if (!dedicatedTransferFamily.has_value()) {
      if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT)
          && !(flags & VK_QUEUE_COMPUTE_BIT)) {
        dedicatedTransferFamily = i;
      }
    }
  }

  if (!universalFamily.has_value()) {
    throw EngineException("Failed to find required queue family index for Universal Queue.");
  }

  VkBool32 presentSupport = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR(device, *universalFamily, surface, &presentSupport);
  if (!presentSupport) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport) {
        dedicatedPresentFamily = i;
        break;
      }
    }
    if (!dedicatedPresentFamily.has_value()) {
      throw EngineException("Failed to find required queue family index for Present Queue");
    }
  }

  return QueueFamilyIndices{
    .universalFamily = *universalFamily,
    .dedicatedPresentFamily = dedicatedPresentFamily,
    .dedicatedTransferFamily = dedicatedTransferFamily,
    .dedicatedComputeFamily = dedicatedComputeFamily};
}

QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device) {
  lib::Buffer<VkQueueFamilyProperties> queueFamilies = getQueueFamilyProperties(device);
  std::optional<uint32_t> universalFamily;
  std::optional<uint32_t> dedicatedTransferFamily;
  std::optional<uint32_t> dedicatedComputeFamily;

  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); i++) {
    const VkQueueFamilyProperties& queueFamily = queueFamilies[i];
    const VkQueueFlags flags = queueFamily.queueFlags;

    if (!universalFamily.has_value()) {
      if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
        universalFamily = i;
      }
    }

    if (!dedicatedComputeFamily.has_value()) {
      if ((flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT)) {
        dedicatedComputeFamily = i;
      }
    }

    if (!dedicatedTransferFamily.has_value()) {
      if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT)
          && !(flags & VK_QUEUE_COMPUTE_BIT)) {
        dedicatedTransferFamily = i;
      }
    }
  }

  if (!universalFamily.has_value()) {
    throw EngineException("Failed to find required queue family indices (Universal and Present).");
  }

  return QueueFamilyIndices{.universalFamily = *universalFamily,
                            .dedicatedTransferFamily = dedicatedTransferFamily,
                            .dedicatedComputeFamily = dedicatedComputeFamily};
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

VkPhysicalDevice getBestPhysicalDevice(
    std::span<const VkPhysicalDevice> devices, VkSurfaceKHR surface) {
  lib::Buffer<uint32_t> rates(devices.size());
  for (auto&& [device, rate] : std::views::zip(devices, rates)) {
    rate = 0;
    const SwapChainSupportDetails swapchainSupportDetails =
        querySwapchainSupportDetails(device, surface);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      rate += 100;
    } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      rate += 75;
    }

    if (!swapchainSupportDetails.formats.empty() && !swapchainSupportDetails.presentModes.empty()) {
      rate += 75;
    }
  }

  const auto maxElementIt = std::max_element(rates.begin(), rates.end());
  if (maxElementIt != rates.end()) {
    return devices[std::distance(rates.begin(), maxElementIt)];
  }

  return VK_NULL_HANDLE;
}

template <typename T>
void chainExtendedField(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice, const Instance& instance,
                               const QueueFamilyIndices& queueFamilyIndices) noexcept
  : _device(physicalDevice), _instance(instance),
    _availableRequestedExtensions(checkDeviceExtensionSupport(physicalDevice)),
    _queueFamilyIndices(queueFamilyIndices) {
  _properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  _properties.pNext = nullptr;
  chainExtendedField(&_properties.pNext, _fsrProperties);
  chainExtendedField(&_properties.pNext, _fdmProperties);
  vkGetPhysicalDeviceProperties2(physicalDevice, &_properties);
}

std::unique_ptr<PhysicalDevice> PhysicalDevice::create(
    const Instance& instance, VkSurfaceKHR surface) {
  const lib::Buffer<VkPhysicalDevice> devices = instance.getAvailablePhysicalDevices();
  const VkPhysicalDevice bestDevice = getBestPhysicalDevice(devices, surface);
  if (bestDevice == VK_NULL_HANDLE) {
    throw EngineException("Failed to find physical device.");
  }
  return std::unique_ptr<PhysicalDevice>(
      new PhysicalDevice(bestDevice, instance, findQueueFamilyIndices(bestDevice, surface)));
}

std::unique_ptr<PhysicalDevice> PhysicalDevice::wrap(
    VkPhysicalDevice physicalDevice, const Instance& instance) {
  if (physicalDevice == VK_NULL_HANDLE) [[unlikely]] {
    throw EngineException("Cannot wrap VK_NULL_HANDLE around PhysicalDevice.");
  }

  return std::unique_ptr<PhysicalDevice>(
      new PhysicalDevice(physicalDevice, instance, findQueueFamilyIndices(physicalDevice)));
}

VkPhysicalDevice PhysicalDevice::getVkPhysicalDevice() const noexcept {
  return _device;
}

const Instance& PhysicalDevice::getInstance() const noexcept {
  return _instance;
}

bool PhysicalDevice::hasAvailableExtension(std::string_view extension) const noexcept {
  return _availableRequestedExtensions.contains(std::string(extension));
}

float PhysicalDevice::getMaxSamplerAnisotropy() const noexcept {
  return _properties.properties.limits.maxSamplerAnisotropy;
}

VkPhysicalDeviceType PhysicalDevice::getPhysicalDeviceType() const noexcept {
  return _properties.properties.deviceType;
}

const VkPhysicalDeviceFragmentDensityMapPropertiesEXT&
PhysicalDevice::getFragmentDensityMapProperties() const noexcept {
  return _fdmProperties;
}

const VkPhysicalDeviceFragmentShadingRatePropertiesKHR&
PhysicalDevice::getFragmentShadingRateProperties() const noexcept {
  return _fsrProperties;
}

lib::Buffer<VkPhysicalDeviceFragmentShadingRateKHR>
PhysicalDevice::getFragmentShadingRates() const noexcept {
  static PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR vkGetPhysicalDeviceFragmentShadingRatesKHR =
      (PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR)vkGetInstanceProcAddr(
          _instance.getVkInstance(), "vkGetPhysicalDeviceFragmentShadingRatesKHR");
  if (vkGetPhysicalDeviceFragmentShadingRatesKHR == nullptr) {
    return lib::Buffer<VkPhysicalDeviceFragmentShadingRateKHR>{};
  }

  uint32_t fsRates;
  vkGetPhysicalDeviceFragmentShadingRatesKHR(_device, &fsRates, nullptr);
  lib::Buffer<VkPhysicalDeviceFragmentShadingRateKHR> fragmentShadingrates(
      fsRates, VkPhysicalDeviceFragmentShadingRateKHR{
                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR});
  vkGetPhysicalDeviceFragmentShadingRatesKHR(_device, &fsRates, fragmentShadingrates.data());
  return fragmentShadingrates;
}

size_t PhysicalDevice::getMemoryAlignment(size_t size) const noexcept {
  const size_t minUboAlignment = _properties.properties.limits.minUniformBufferOffsetAlignment;
  return minUboAlignment > 0 ? (size + minUboAlignment - 1) & ~(minUboAlignment - 1) : size;
}

size_t PhysicalDevice::getStagingAlignment() const noexcept {
  return std::max(_properties.properties.limits.minTexelBufferOffsetAlignment,
                  _properties.properties.limits.optimalBufferCopyOffsetAlignment);
}

const QueueFamilyIndices& PhysicalDevice::getQueueFamilyIndices() const noexcept {
  return _queueFamilyIndices;
}

const SwapChainSupportDetails PhysicalDevice::getSwapchainSupportDetails(
    VkSurfaceKHR surface) const {
  return querySwapchainSupportDetails(_device, surface);
}
