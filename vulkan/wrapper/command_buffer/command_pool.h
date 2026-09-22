#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/logical_device/logical_device.h"

class CommandBuffer;

class CommandPool : public std::enable_shared_from_this<const CommandPool> {
  CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool) noexcept;

public:
  static std::unique_ptr<CommandPool> createPtr(
      const LogicalDevice& logicalDevice, const VkCommandPoolCreateInfo& createInfo);

  ~CommandPool();

  CommandBuffer createCommandBuffer(VkCommandBufferLevel level) const;

  std::vector<CommandBuffer> createCommandBuffers(VkCommandBufferLevel level, uint32_t count) const;

  // The definition of this template lives in command_buffer.h (it needs the
  // full CommandBuffer type). Include command_buffer.h in any translation unit that calls it.
  template <size_t COUNT>
  std::array<CommandBuffer, COUNT> createCommandBuffers(VkCommandBufferLevel level) const;

  void reset() const;

  VkCommandPool getVkCommandPool() const noexcept;

  const LogicalDevice& getLogicalDevice() const;

private:
  VkCommandPool _commandPool;
  const LogicalDevice& _logicalDevice;
};

class CommandPoolBuilder {
public:
  CommandPoolBuilder&& withQueueFamilyIndex(uint32_t index) && noexcept;

  CommandPoolBuilder&& withFlags(VkCommandPoolCreateFlags flags) && noexcept;

  std::unique_ptr<CommandPool> build(const LogicalDevice& logicalDevice) const;

private:
  VkCommandPoolCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueFamilyIndex = 0};

  void* _pNext = nullptr;
};
