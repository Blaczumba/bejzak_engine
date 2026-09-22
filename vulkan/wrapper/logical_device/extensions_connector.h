#pragma once

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

  ExtensionsConnector& withFragmentShadingRateExtension();

  ExtensionsConnector& withFragmentDensityMapExtension();

  ExtensionsConnector& withSynchronization2();

  void* getNext() const;

private:
  const PhysicalDevice& _physicalDevice;

  void* _next = nullptr;
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT _indexTypeUint8;
  VkPhysicalDeviceBufferDeviceAddressFeatures _bufferDeviceAddress;
  VkPhysicalDeviceDescriptorIndexingFeatures _descriptorIndexing;
  VkPhysicalDeviceInheritedViewportScissorFeaturesNV _inheritedViewportScissor;
  VkPhysicalDeviceMultiviewFeatures _multiview;
  VkPhysicalDevice8BitStorageFeatures _storage8Bit;
  VkPhysicalDevice16BitStorageFeatures _storage16Bit;
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR _fragmentShadingRate;
  VkPhysicalDeviceFragmentDensityMapFeaturesEXT _fragmentDensityMap;
  VkPhysicalDeviceSynchronization2Features _synchronization2;
};
