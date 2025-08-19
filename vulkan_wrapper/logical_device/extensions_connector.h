#pragma once

#include "vulkan_wrapper/physical_device/physical_device.h"

struct DeviceFeatures {
  void* next = nullptr;
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8;
  VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress;
  VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing;
  VkPhysicalDeviceInheritedViewportScissorFeaturesNV inheritedViewportScissor;
};

void chainExtensionIndexTypeUint8(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice);

void chainExtensionBufferDeviceAddress(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice);

void chainExtensionInheritedViewportScissor(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice);

void chainExtensionDescriptorIndexing(
    DeviceFeatures& deviceFeatures, const PhysicalDevice& physicalDevice);
