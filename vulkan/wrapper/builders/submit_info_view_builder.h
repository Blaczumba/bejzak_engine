#pragma once

#include <span>
#include <vulkan/vulkan.h>

class SubmitInfoViewBuilder {
public:
  SubmitInfoViewBuilder() noexcept = default;

  ~SubmitInfoViewBuilder() = default;

  SubmitInfoViewBuilder& withWaitSemaphores(
      std::span<const VkSemaphore> waitSemaphores,
      std::span<const VkPipelineStageFlags> waitDstStageMasks) noexcept;

  SubmitInfoViewBuilder& withSignalSemaphores(
      std::span<const VkSemaphore> signalSemaphores) noexcept;

  SubmitInfoViewBuilder& withCommandBuffers(
      std::span<const VkCommandBuffer> commandBuffers) noexcept;

  VkResult submitQueue(VkQueue queue, VkFence fence) noexcept;

private:
  VkSubmitInfo _submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};

  void* _pNext = nullptr;
};
