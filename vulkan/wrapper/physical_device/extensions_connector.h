#pragma once

#include <optional>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"

class ExtensionsConnector {
public:
  ExtensionsConnector(const PhysicalDevice& physicalDevice) noexcept;

  ExtensionsConnector(const ExtensionsConnector& other) = delete;

  ExtensionsConnector(ExtensionsConnector&& other) = delete;

  ~ExtensionsConnector() = default;

  ExtensionsConnector& withIndexTypeUint8Extension();

  ExtensionsConnector& withBufferDeviceAddressExtension();

  ExtensionsConnector& withInheritedViewportScissorExtension();

  ExtensionsConnector& withDescriptorIndexingExtension();

  ExtensionsConnector& withMultiviewExtension();

  ExtensionsConnector& withStorage8BitExtension();

  ExtensionsConnector& withStorage16BitExtension();

  ExtensionsConnector& withFragmentShadingRateExtension(
      const VkPhysicalDeviceFragmentShadingRateFeaturesKHR& features);

  ExtensionsConnector& withFragmentDensityMapExtension(
      const VkPhysicalDeviceFragmentDensityMapFeaturesEXT& features);

  ExtensionsConnector& withSynchronization2();

  VkPhysicalDeviceFeatures2 getVkPhysicalDeviceFeatures2() const;

  lib::Buffer<const char*> getAvailableRequestedDeviceExtensions() noexcept;

private:
  const PhysicalDevice& _physicalDevice;

  void* _next = nullptr;
  std::optional<VkPhysicalDeviceIndexTypeUint8FeaturesEXT> _indexTypeUint8;
  std::optional<VkPhysicalDeviceBufferDeviceAddressFeatures> _bufferDeviceAddress;
  std::optional<VkPhysicalDeviceDescriptorIndexingFeatures> _descriptorIndexing;
  std::optional<VkPhysicalDeviceInheritedViewportScissorFeaturesNV> _inheritedViewportScissor;
  std::optional<VkPhysicalDeviceMultiviewFeatures> _multiview;
  std::optional<VkPhysicalDevice8BitStorageFeatures> _storage8Bit;
  std::optional<VkPhysicalDevice16BitStorageFeatures> _storage16Bit;
  std::optional<VkPhysicalDeviceFragmentShadingRateFeaturesKHR> _fragmentShadingRate;
  std::optional<VkPhysicalDeviceFragmentDensityMapFeaturesEXT> _fragmentDensityMap;
  std::optional<VkPhysicalDeviceSynchronization2Features> _synchronization2;

  std::unordered_set<const char*> _availableRequestedDeviceExtensions;
};
