#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "common/status/status.h"
#include "vulkan_wrapper/surface/surface.h"

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
  VkPhysicalDeviceProperties _properties;

  const Surface& _surface;

  const std::unordered_set<std::string_view> _availableRequestedExtensions;  // TODO: change to flat
                                                                             // hash set

  PhysicalDevice(VkPhysicalDevice physicalDevice, const Surface& surface);

public:
  ~PhysicalDevice() = default;

  static ErrorOr<std::unique_ptr<PhysicalDevice>> create(const Surface& surface);

  VkPhysicalDevice getVkPhysicalDevice() const;

  const Surface& getSurface() const;

  bool hasAvailableExtension(std::string_view extension) const;

  float getMaxSamplerAnisotropy() const;

  size_t getMemoryAlignment(size_t size) const;

  lib::Buffer<const char*> getAvailableExtensions() const;

  QueueFamilyIndices getQueueFamilyIndices() const;

  SwapChainSupportDetails getSwapchainSupportDetails() const;
};
