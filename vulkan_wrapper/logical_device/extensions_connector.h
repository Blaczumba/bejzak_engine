#pragma once

#include "vulkan_wrapper/physical_device/physical_device.h"

void chainExtensionIndexTypeUint8(void *next, const PhysicalDevice& physicalDevice);

void chainExtensionBufferDeviceAddress(void *next, const PhysicalDevice& physicalDevice);

void chainExtensionInheritedViewportScissor(void *next, const PhysicalDevice& physicalDevice);

void chainExtensionDescriptorIndexing(void *next, const PhysicalDevice& physicalDevice);
