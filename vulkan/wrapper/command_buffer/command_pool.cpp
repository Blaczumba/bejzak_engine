#include "vulkan/wrapper/command_buffer/command_pool.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/util/check.h"

CommandPool::CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool) noexcept
  : _logicalDevice(logicalDevice), _commandPool(commandPool) {}

std::unique_ptr<CommandPool> CommandPool::createPtr(
    const LogicalDevice& logicalDevice, const VkCommandPoolCreateInfo& createInfo) {
  VkCommandPool commandPool;
  CHECK_VKCMD(vkCreateCommandPool(logicalDevice.getVkDevice(), &createInfo, nullptr, &commandPool),
              "Failed to create VkCommandPool in CommandPool::create.");
  return std::unique_ptr<CommandPool>(new CommandPool(logicalDevice, commandPool));
}

CommandPool::~CommandPool() {
  _logicalDevice.destroyResource([commandPool = _commandPool](DestroyerContext context) {
    vkDestroyCommandPool(context.device, commandPool, context.allocationCallbacks);
  });
}

CommandBuffer CommandPool::createCommandBuffer(VkCommandBufferLevel level) const {
  return CommandBuffer::create(shared_from_this(), level);
}

std::vector<CommandBuffer> CommandPool::createCommandBuffers(
    VkCommandBufferLevel level, uint32_t count) const {
  return CommandBuffer::create(shared_from_this(), level, count);
}

void CommandPool::reset() const {
  vkResetCommandPool(_logicalDevice.getVkDevice(), _commandPool, 0);
}

VkCommandPool CommandPool::getVkCommandPool() const noexcept {
  return _commandPool;
}

const LogicalDevice& CommandPool::getLogicalDevice() const {
  return _logicalDevice;
}

CommandPoolBuilder&& CommandPoolBuilder::withQueueFamilyIndex(uint32_t index) && noexcept {
  _createInfo.queueFamilyIndex = index;
  return std::move(*this);
}

CommandPoolBuilder&& CommandPoolBuilder::withFlags(VkCommandPoolCreateFlags flags) && noexcept {
  _createInfo.flags = flags;
  return std::move(*this);
}

std::unique_ptr<CommandPool> CommandPoolBuilder::build(const LogicalDevice& logicalDevice) const {
  return CommandPool::createPtr(logicalDevice, _createInfo);
}
