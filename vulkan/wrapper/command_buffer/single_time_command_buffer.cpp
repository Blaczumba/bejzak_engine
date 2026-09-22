#include "vulkan/wrapper/command_buffer/single_time_command_buffer.h"

#include "vulkan/wrapper/builders/submit_info_builder.h"
#include "vulkan/wrapper/synchronization/fence.h"

SingleTimeCommandBuffer::SingleTimeCommandBuffer(
    const CommandPool& commandPool, QueueType queueType)
  : CommandBuffer(
        CommandBuffer::create(commandPool.shared_from_this(), VK_COMMAND_BUFFER_LEVEL_PRIMARY)),
    _fence(FenceBuilder().build(commandPool.getLogicalDevice())), _queueType(queueType) {
  CHECK_VKCMD(
      BeginInfoBuilder().beginCommandBuffer(*this, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT),
      "Failed to begin single time command buffer.");
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer() {
  CHECK_VKCMD(end(), "Failed to end single time command buffer.");
  CHECK_VKCMD(SubmitInfoBuilder()
                  .withCommandBuffers({_commandBuffer})
                  .submitQueue(
                      _commandPool->getLogicalDevice().getVkQueue(_queueType), _fence.getVkFence()),
              "Failed to submit single time command buffer.");
  CHECK_VKCMD(_fence.wait(), "Failed to wait for fence.");
}
