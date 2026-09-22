#pragma once

#include <initializer_list>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

class SubmitInfoBuilder {
public:
  SubmitInfoBuilder() noexcept = default;

  ~SubmitInfoBuilder() = default;

  SubmitInfoBuilder& withWaitSemaphores(std::span<const VkSemaphore> waitSemaphores,
                                        std::span<const VkPipelineStageFlags> waitDstStageMasks);

  SubmitInfoBuilder& withWaitSemaphores(
      std::initializer_list<VkSemaphore> waitSemaphores,
      std::initializer_list<VkPipelineStageFlags> waitDstStageMasks);

  SubmitInfoBuilder& withSignalSemaphores(std::span<const VkSemaphore> signalSemaphores);

  SubmitInfoBuilder& withSignalSemaphores(std::initializer_list<VkSemaphore> signalSemaphores);

  SubmitInfoBuilder& withCommandBuffers(std::span<const VkCommandBuffer> commandBuffers);

  SubmitInfoBuilder& withCommandBuffers(std::initializer_list<VkCommandBuffer> commandBuffers);

  VkResult submitQueue(VkQueue queue, VkFence fence) const noexcept;

private:
  std::vector<VkSemaphore> _waitSemaphores;
  std::vector<VkPipelineStageFlags> _waitDstStageMasks;
  std::vector<VkSemaphore> _signalSemaphores;
  std::vector<VkCommandBuffer> _commandBuffers;

  void* _pNext = nullptr;
};
