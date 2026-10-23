#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/instance/instance.h"

struct QueueFamilyIndices {
  uint32_t universalFamily;  // Graphics + Compute + Transfer + Present
  std::optional<uint32_t> dedicatedPresentFamily;
  std::optional<uint32_t> dedicatedTransferFamily;
  std::optional<uint32_t> dedicatedComputeFamily;
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  lib::Buffer<VkSurfaceFormatKHR> formats;
  lib::Buffer<VkPresentModeKHR> presentModes;
};

class PhysicalDevice {
  PhysicalDevice(VkPhysicalDevice physicalDevice, const Instance& instance,
                 const QueueFamilyIndices& queueFamilyIndices) noexcept;

public:
  ~PhysicalDevice() = default;

  static std::unique_ptr<PhysicalDevice> create(const Instance& instance, VkSurfaceKHR surface);

  static std::unique_ptr<PhysicalDevice> wrap(
      VkPhysicalDevice physicalDevice, const Instance& instance);

  VkPhysicalDevice getVkPhysicalDevice() const noexcept;

  const Instance& getInstance() const noexcept;

  bool hasAvailableExtension(std::string_view extension) const noexcept;

  float getMaxSamplerAnisotropy() const noexcept;

  VkPhysicalDeviceType getPhysicalDeviceType() const noexcept;

  VkPhysicalDeviceFragmentDensityMapPropertiesEXT getFragmentDensityMapProperties() const noexcept;

  VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM
  getFragmentDensityMapOffsetProperties() const noexcept;

  VkPhysicalDeviceFragmentShadingRatePropertiesKHR
  getFragmentShadingRateProperties() const noexcept;

  lib::Buffer<VkPhysicalDeviceFragmentShadingRateKHR> getFragmentShadingRates() const noexcept;

  size_t getMemoryAlignment(size_t size) const noexcept;

  size_t getStagingAlignment() const noexcept;

  const QueueFamilyIndices& getQueueFamilyIndices() const noexcept;

  const SwapChainSupportDetails getSwapchainSupportDetails(VkSurfaceKHR surface) const;

private:
  VkPhysicalDevice _device = VK_NULL_HANDLE;

  const Instance& _instance;

  VkPhysicalDeviceProperties2 _properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

  QueueFamilyIndices _queueFamilyIndices;

  const std::unordered_set<std::string> _availableRequestedExtensions;
};
