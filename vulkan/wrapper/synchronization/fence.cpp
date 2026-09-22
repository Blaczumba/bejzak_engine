#include "vulkan/wrapper/synchronization/fence.h"

#include <cstdint>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

Fence::Fence(const LogicalDevice& logicalDevice, VkFence fence)
  : _fence(fence), _logicalDevice(&logicalDevice) {}

Fence Fence::create(const LogicalDevice& logicalDevice, const VkFenceCreateInfo& createInfo) {
  VkFence fence;
  CHECK_VKCMD(vkCreateFence(logicalDevice.getVkDevice(), &createInfo, nullptr, &fence),
              "Failed to create VkFence.");
  return Fence(logicalDevice, fence);
}

Fence::Fence(Fence&& other) noexcept
  : _fence(std::exchange(other._fence, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(other._logicalDevice, nullptr)) {}

Fence& Fence::operator=(Fence&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (_fence != VK_NULL_HANDLE) {
    destroy();
  }

  _fence = std::exchange(other._fence, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(other._logicalDevice, nullptr);
  return *this;
}

Fence::~Fence() {
  if (_fence != VK_NULL_HANDLE) {
    destroy();
    _fence = VK_NULL_HANDLE;
  }
}

VkResult Fence::wait(uint64_t timeout) const {
  return vkWaitForFences(_logicalDevice->getVkDevice(), 1, &_fence, VK_FALSE, timeout);
}

VkResult Fence::reset() const {
  return vkResetFences(_logicalDevice->getVkDevice(), 1, &_fence);
}

VkFence Fence::getVkFence() const noexcept {
  return _fence;
}

void Fence::destroy() {
  _logicalDevice->destroyResource([fence = _fence](DestroyerContext context) {
    vkDestroyFence(context.device, fence, context.allocationCallbacks);
  });
}

Fence FenceBuilder::build(const LogicalDevice& logicalDevice, VkFenceCreateFlags flags) {
  _flags = flags;

  const VkFenceCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = _pNext, .flags = flags};
  return Fence::create(logicalDevice, createInfo);
}
