#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"

std::vector<VkDeviceQueueCreateInfo> getDeviceQueueCreateInfos(const QueueFamilyIndices& indices);
