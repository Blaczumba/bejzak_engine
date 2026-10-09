#pragma once

#include <initializer_list>
#include <lib/buffer/buffer.h>
#include <span>
#include <vulkan/vulkan.h>

class SubmitInfoOwningBuilder {
public:
  SubmitInfoOwningBuilder() noexcept = default;

  ~SubmitInfoOwningBuilder() = default;

  SubmitInfoOwningBuilder& withWaitSemaphores(
      std::span<const VkSemaphore> waitSemaphores,
      std::span<const VkPipelineStageFlags> waitDstStageMasks);

  SubmitInfoOwningBuilder& withWaitSemaphores(
      std::initializer_list<VkSemaphore> waitSemaphores,
      std::initializer_list<VkPipelineStageFlags> waitDstStageMasks);

  SubmitInfoOwningBuilder& withSignalSemaphores(std::span<const VkSemaphore> signalSemaphores);

  SubmitInfoOwningBuilder& withSignalSemaphores(
      std::initializer_list<VkSemaphore> signalSemaphores);

  SubmitInfoOwningBuilder& withCommandBuffers(std::span<const VkCommandBuffer> commandBuffers);

  SubmitInfoOwningBuilder& withCommandBuffers(
      std::initializer_list<VkCommandBuffer> commandBuffers);

  VkResult submitQueue(VkQueue queue, VkFence fence) const noexcept;

private:
  lib::Buffer<VkSemaphore> _waitSemaphores;
  lib::Buffer<VkPipelineStageFlags> _waitDstStageMasks;
  lib::Buffer<VkSemaphore> _signalSemaphores;
  lib::Buffer<VkCommandBuffer> _commandBuffers;

  VkSubmitInfo _submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
};

class SubmitInfoNonOwningBuilder {
public:
  SubmitInfoNonOwningBuilder() noexcept = default;

  ~SubmitInfoNonOwningBuilder() = default;

  SubmitInfoNonOwningBuilder& withWaitSemaphores(
      std::span<const VkSemaphore> waitSemaphores,
      std::span<const VkPipelineStageFlags> waitDstStageMasks) noexcept;

  SubmitInfoNonOwningBuilder& withSignalSemaphores(
      std::span<const VkSemaphore> signalSemaphores) noexcept;

  SubmitInfoNonOwningBuilder& withCommandBuffers(
      std::span<const VkCommandBuffer> commandBuffers) noexcept;

  VkResult submitQueue(VkQueue queue, VkFence fence) const noexcept;

private:
  VkSubmitInfo _submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
};
