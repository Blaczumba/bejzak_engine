#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

class CommandBuffer;

class CommandPool : public std::enable_shared_from_this<const CommandPool> {
  VkCommandPool _commandPool;
  const LogicalDevice& _logicalDevice;

  CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool);

public:
  static ErrorOr<std::unique_ptr<CommandPool>> create(
      const LogicalDevice& logicalDevice, VkCommandPoolCreateFlags flags = 0);

  ~CommandPool();

  ErrorOr<CommandBuffer> createCommandBuffer(VkCommandBufferLevel level) const;

  ErrorOr<std::vector<CommandBuffer>> createCommandBuffers(
      VkCommandBufferLevel level, uint32_t count) const;

  template <size_t COUNT>
  ErrorOr<std::array<CommandBuffer, COUNT>> createCommandBuffers(VkCommandBufferLevel level) const;

  void reset() const;

  VkCommandPool getVkCommandPool() const;

  const LogicalDevice& getLogicalDevice() const;
};

class CommandBuffer {
  VkCommandBuffer _commandBuffer;
  std::shared_ptr<const CommandPool> _commandPool;

  VkCommandBufferLevel _level;

  CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool,
                VkCommandBuffer commandBuffer, VkCommandBufferLevel level);

public:
  CommandBuffer();

  static ErrorOr<CommandBuffer> create(
      const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level);

  template <size_t COUNT>
  static ErrorOr<std::array<CommandBuffer, COUNT>> create(
      const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level);

  static ErrorOr<std::vector<CommandBuffer>> create(
      const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level,
      uint32_t count);

  CommandBuffer(CommandBuffer&&) noexcept;

  CommandBuffer& operator=(CommandBuffer&&) noexcept;

  ~CommandBuffer();

  Status beginRenderPass(const Framebuffer& framebuffer) const;

  void endRenderPass() const;

  Status beginAsPrimary(uint32_t subpassIndex = 0) const;

  Status beginAsSecondary(
      const Framebuffer& framebuffer,
      const VkCommandBufferInheritanceViewportScissorInfoNV* scissorViewportInheritance = nullptr,
      uint32_t subpassIndex = 0) const;

  VkResult end() const;

  Status executeSecondaryCommandBuffers(
      std::initializer_list<VkCommandBuffer> commandBuffers) const;

  Status submit(QueueType type, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore,
                const VkFence waitFence) const;

  void resetCommandBuffer() const;

  VkCommandBuffer getVkCommandBuffer() const;
};

template <size_t COUNT>
ErrorOr<std::array<CommandBuffer, COUNT>> CommandPool::createCommandBuffers(
    VkCommandBufferLevel level) const {
  return CommandBuffer::create<COUNT>(shared_from_this(), level);
}

namespace {

VkResult createCommandBuffers(
    VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level,
    std::span<VkCommandBuffer> outCommandBuffers) {
  const VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = level,
    .commandBufferCount = static_cast<uint32_t>(outCommandBuffers.size()),
  };
  return vkAllocateCommandBuffers(device, &allocInfo, outCommandBuffers.data());
}

}  // namespace

template <size_t COUNT>
ErrorOr<std::array<CommandBuffer, COUNT>> CommandBuffer::create(
    const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level) {
  VkCommandBuffer vkCommandBuffers[COUNT];
  CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(),
                                   commandPool->getVkCommandPool(), level, vkCommandBuffers));

  std::array<CommandBuffer, COUNT> commandBuffers;
  std::transform(std::cbegin(vkCommandBuffers), std::cend(vkCommandBuffers), commandBuffers.begin(),
                 [&commandPool, level](VkCommandBuffer commandBuffer) {
                   return CommandBuffer(commandPool, commandBuffer, level);
                 });
  return commandBuffers;
}

class SingleTimeCommandBuffer {
  VkCommandBuffer _commandBuffer;
  VkFence _fence;
  const QueueType _queueType;

  const CommandPool& _commandPool;

public:
  SingleTimeCommandBuffer(
      const CommandPool& commandPool, QueueType queueType = QueueType::GRAPHICS);

  ~SingleTimeCommandBuffer();

  VkCommandBuffer getCommandBuffer() const;
};
