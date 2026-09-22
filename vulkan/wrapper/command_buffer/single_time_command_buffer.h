#pragma once

#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/synchronization/fence.h"

class SingleTimeCommandBuffer final : public CommandBuffer {
public:
  explicit SingleTimeCommandBuffer(
      const CommandPool& commandPool, QueueType queueType = QueueType::GRAPHICS);

  ~SingleTimeCommandBuffer();

  SingleTimeCommandBuffer(const SingleTimeCommandBuffer&) = delete;

  SingleTimeCommandBuffer& operator=(const SingleTimeCommandBuffer&) = delete;

  SingleTimeCommandBuffer(SingleTimeCommandBuffer&&) = delete;

  SingleTimeCommandBuffer& operator=(SingleTimeCommandBuffer&&) = delete;

private:
  Fence _fence;
  const QueueType _queueType;
};
