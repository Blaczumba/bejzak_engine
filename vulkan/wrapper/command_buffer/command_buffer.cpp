#include "command_buffer.h"

#include <iterator>

#include "vulkan/wrapper/util/check.h"

CommandPool::CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool)
  : _logicalDevice(logicalDevice), _commandPool(commandPool) {}

ErrorOr<std::unique_ptr<CommandPool>> CommandPool::create(
    const LogicalDevice& logicalDevice, VkCommandPoolCreateFlags flags) {
  const VkCommandPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = flags,
    .queueFamilyIndex = *logicalDevice.getPhysicalDevice().getQueueFamilyIndices().graphicsFamily};

  VkCommandPool commandPool;
  CHECK_VKCMD(vkCreateCommandPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &commandPool));
  return std::unique_ptr<CommandPool>(new CommandPool(logicalDevice, commandPool));
}

CommandPool::~CommandPool() {
  vkDestroyCommandPool(_logicalDevice.getVkDevice(), _commandPool, nullptr);
}
ErrorOr<CommandBuffer> CommandPool::createCommandBuffer(VkCommandBufferLevel level) const {
  return CommandBuffer::create(shared_from_this(), level);
}

ErrorOr<std::vector<CommandBuffer>> CommandPool::createCommandBuffers(
    VkCommandBufferLevel level, uint32_t count) const {
  return CommandBuffer::create(shared_from_this(), level, count);
}

void CommandPool::reset() const {
  vkResetCommandPool(_logicalDevice.getVkDevice(), _commandPool, 0);
}

VkCommandPool CommandPool::getVkCommandPool() const {
  return _commandPool;
}

const LogicalDevice& CommandPool::getLogicalDevice() const {
  return _logicalDevice;
}

CommandBuffer::CommandBuffer() : _commandBuffer(VK_NULL_HANDLE) {}

CommandBuffer::CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool,
                             VkCommandBuffer commandBuffer, VkCommandBufferLevel level)
  : _commandPool(commandPool), _commandBuffer(commandBuffer), _level(level) {}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
  : _commandPool(std::move(other._commandPool)),
    _commandBuffer(std::exchange(other._commandBuffer, VK_NULL_HANDLE)), _level(other._level) {}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  _commandPool = std::move(other._commandPool);
  _commandBuffer = std::exchange(other._commandBuffer, VK_NULL_HANDLE);
  _level = other._level;
  return *this;
}

CommandBuffer::~CommandBuffer() {
  if (_commandBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(_commandPool->getLogicalDevice().getVkDevice(),
                         _commandPool->getVkCommandPool(), 1, &_commandBuffer);
  }
}

ErrorOr<CommandBuffer> CommandBuffer::create(
    const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level) {
  VkCommandBuffer commandBuffer;
  CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(),
                                   commandPool->getVkCommandPool(), level, {&commandBuffer, 1}));
  return CommandBuffer(commandPool, commandBuffer, level);
}

ErrorOr<std::vector<CommandBuffer>> CommandBuffer::create(
    const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level,
    uint32_t count) {
  lib::Buffer<VkCommandBuffer> vkCommandBuffers(count);
  CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(),
                                   commandPool->getVkCommandPool(), level, vkCommandBuffers));
  std::vector<CommandBuffer> commandBuffers;
  commandBuffers.reserve(vkCommandBuffers.size());
  std::transform(
      std::cbegin(vkCommandBuffers), std::cend(vkCommandBuffers),
      std::back_inserter(commandBuffers), [&commandPool, level](VkCommandBuffer commandBuffer) {
        return CommandBuffer(commandPool, commandBuffer, level);
      });
  return commandBuffers;
}

Status CommandBuffer::beginRenderPass(const Framebuffer& framebuffer) const {
  if (_level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }
  const Renderpass& renderpass = framebuffer.getRenderpass();
  std::span<const VkClearValue> clearValues = renderpass.getAttachmentsLayout().getVkClearValues();
  const VkRenderPassBeginInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = renderpass.getVkRenderPass(),
    .framebuffer = framebuffer.getVkFramebuffer(),
    .renderArea = {.offset = {0, 0}, .extent = framebuffer.getVkExtent()},
    .clearValueCount = static_cast<uint32_t>(clearValues.size()),
    .pClearValues = clearValues.data()
  };
  vkCmdSetViewport(_commandBuffer, 0, 1, &framebuffer.getViewport());
  vkCmdSetScissor(_commandBuffer, 0, 1, &framebuffer.getScissor());
  vkCmdBeginRenderPass(
      _commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
  return StatusOk();
}

void CommandBuffer::endRenderPass() const {
  vkCmdEndRenderPass(_commandBuffer);
}

Status CommandBuffer::beginAsPrimary(uint32_t subpassIndex) const {
  if (_level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }

  const VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  CHECK_VKCMD(vkBeginCommandBuffer(_commandBuffer, &beginInfo));
  return StatusOk();
}

Status CommandBuffer::beginAsSecondary(
    const Framebuffer& framebuffer,
    const VkCommandBufferInheritanceViewportScissorInfoNV* scissorViewportInheritance,
    uint32_t subpassIndex) const {
  if (_level != VK_COMMAND_BUFFER_LEVEL_SECONDARY) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }

  const VkCommandBufferInheritanceInfo inheritanceInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    .pNext = scissorViewportInheritance,
    .renderPass = framebuffer.getRenderpass().getVkRenderPass(),
    .subpass = subpassIndex,
    .framebuffer = framebuffer.getVkFramebuffer()};

  const VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
             | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = &inheritanceInfo};

  CHECK_VKCMD(vkBeginCommandBuffer(_commandBuffer, &beginInfo));
  return StatusOk();
}

VkResult CommandBuffer::end() const {
  return vkEndCommandBuffer(_commandBuffer);
}

Status CommandBuffer::executeSecondaryCommandBuffers(
    std::initializer_list<VkCommandBuffer> commandBuffers) const {
  if (_level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }
  vkCmdExecuteCommands(
      _commandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.begin());
  return StatusOk();
}

Status CommandBuffer::submit(QueueType type, const VkSemaphore waitSemaphore,
                             const VkSemaphore signalSemaphore, const VkFence waitFence) const {
  if (_level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) [[unlikely]] {
    return Error(EngineError::FLAG_NOT_SPECIFIED);
  }
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {waitSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  if (waitSemaphore != VK_NULL_HANDLE) {
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
  }

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &_commandBuffer;

  VkSemaphore signalSemaphores[] = {signalSemaphore};
  if (signalSemaphore != VK_NULL_HANDLE) {
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
  }

  const LogicalDevice& logicalDevice = _commandPool->getLogicalDevice();
  if (waitFence != VK_NULL_HANDLE) {
    CHECK_VKCMD(vkResetFences(logicalDevice.getVkDevice(), 1, &waitFence));
  }

  CHECK_VKCMD(vkQueueSubmit(logicalDevice.getVkQueue(type), 1, &submitInfo, waitFence));
  return StatusOk();
}

void CommandBuffer::resetCommandBuffer() const {
  vkResetCommandBuffer(_commandBuffer, 0);
}

VkCommandBuffer CommandBuffer::getVkCommandBuffer() const {
  return _commandBuffer;
}

SingleTimeCommandBuffer::SingleTimeCommandBuffer(
    const CommandPool& commandPool, QueueType queueType)
  : _commandPool(commandPool), _queueType(queueType) {
  const VkDevice device = _commandPool.getLogicalDevice().getVkDevice();
  const VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

  vkCreateFence(device, &fenceInfo, nullptr, &_fence);

  const VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = _commandPool.getVkCommandPool(),
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1};

  vkAllocateCommandBuffers(device, &allocInfo, &_commandBuffer);

  const VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer() {
  const LogicalDevice& logicalDevice = _commandPool.getLogicalDevice();
  const VkDevice device = logicalDevice.getVkDevice();

  vkEndCommandBuffer(_commandBuffer);

  const VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &_commandBuffer};

  vkQueueSubmit(logicalDevice.getVkQueue(_queueType), 1, &submitInfo, _fence);
  vkWaitForFences(device, 1, &_fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(device, _fence, nullptr);

  vkFreeCommandBuffers(device, _commandPool.getVkCommandPool(), 1, &_commandBuffer);
}

VkCommandBuffer SingleTimeCommandBuffer::getCommandBuffer() const {
  return _commandBuffer;
}
