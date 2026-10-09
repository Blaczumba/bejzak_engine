#include "vulkan/wrapper/logical_device/lib.h"

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/physical_device/physical_device.h"

std::vector<VkDeviceQueueCreateInfo> getDeviceQueueCreateInfos(const QueueFamilyIndices& indices) {
  static constexpr float queuePriority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  queueCreateInfos.reserve(4);
  for (std::optional<uint32_t> familyIndex :
       {std::optional<uint32_t>(indices.universalFamily), indices.dedicatedPresentFamily,
        indices.dedicatedTransferFamily, indices.dedicatedComputeFamily}) {
    if (familyIndex.has_value()) {
      queueCreateInfos.push_back(VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = *familyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority});
    }
  }
  return queueCreateInfos;
}
