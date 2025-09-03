#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "common/status/status.h"
#include "vulkan_wrapper/instance/instance.h"

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  std::optional<uint32_t> computeFamily;
  std::optional<uint32_t> transferFamily;
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  lib::Buffer<VkSurfaceFormatKHR> formats;
  lib::Buffer<VkPresentModeKHR> presentModes;
};

class PhysicalDevice {
  VkPhysicalDevice _device;

  const Instance& _instance;

  VkPhysicalDeviceProperties _properties;
  QueueFamilyIndices _queueFamilyIndices;

  const std::unordered_set<std::string_view> _availableRequestedExtensions;

  PhysicalDevice(VkPhysicalDevice physicalDevice, const Instance& instance,
                 const QueueFamilyIndices& queueFamilyIndices);

public:
  ~PhysicalDevice() = default;

  static ErrorOr<std::unique_ptr<PhysicalDevice>> create(
      const Instance& instance, VkSurfaceKHR surface);

  static ErrorOr<std::unique_ptr<PhysicalDevice>> wrap(
      VkPhysicalDevice physicalDevice, const Instance& instance);

  VkPhysicalDevice getVkPhysicalDevice() const;

  const Instance& getInstance() const;

  bool hasAvailableExtension(std::string_view extension) const;

  float getMaxSamplerAnisotropy() const;

  size_t getMemoryAlignment(size_t size) const;

  lib::Buffer<const char*> getAvailableExtensions() const;

  const QueueFamilyIndices& getQueueFamilyIndices() const;

  const SwapChainSupportDetails getSwapchainSupportDetails(VkSurfaceKHR surface) const;
};
