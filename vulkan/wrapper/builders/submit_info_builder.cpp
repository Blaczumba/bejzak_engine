#include "vulkan/wrapper/builders/submit_info_builder.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"

SubmitInfoBuilder& SubmitInfoBuilder::withWaitSemaphores(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMasks) {
  if (waitSemaphores.size() != waitDstStageMasks.size()) [[unlikely]] {
    throw EngineException(
        "Wait semaphores and wait destination stage masks must have the same size.");
  }
  _waitSemaphores.assign_range(waitSemaphores);
  _waitDstStageMasks.assign_range(waitDstStageMasks);
  return *this;
}

SubmitInfoBuilder& SubmitInfoBuilder::withWaitSemaphores(
    std::initializer_list<VkSemaphore> waitSemaphores,
    std::initializer_list<VkPipelineStageFlags> waitDstStageMasks) {
  if (waitSemaphores.size() != waitDstStageMasks.size()) [[unlikely]] {
    throw EngineException(
        "Wait semaphores and wait destination stage masks must have the same size.");
  }
  _waitSemaphores.assign_range(waitSemaphores);
  _waitDstStageMasks.assign_range(waitDstStageMasks);
  return *this;
}

SubmitInfoBuilder& SubmitInfoBuilder::withSignalSemaphores(
    std::span<const VkSemaphore> signalSemaphores) {
  _signalSemaphores.assign_range(signalSemaphores);
  return *this;
}

SubmitInfoBuilder& SubmitInfoBuilder::withSignalSemaphores(
    std::initializer_list<VkSemaphore> signalSemaphores) {
  _signalSemaphores.assign_range(signalSemaphores);
  return *this;
}

SubmitInfoBuilder& SubmitInfoBuilder::withCommandBuffers(
    std::span<const VkCommandBuffer> commandBuffers) {
  _commandBuffers.assign_range(commandBuffers);
  return *this;
}

SubmitInfoBuilder& SubmitInfoBuilder::withCommandBuffers(
    std::initializer_list<VkCommandBuffer> commandBuffers) {
  _commandBuffers.assign_range(commandBuffers);
  return *this;
}

VkResult SubmitInfoBuilder::submitQueue(VkQueue queue, VkFence fence) const noexcept {
  const VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = _pNext,
    .waitSemaphoreCount = static_cast<uint32_t>(_waitSemaphores.size()),
    .pWaitSemaphores = _waitSemaphores.data(),
    .pWaitDstStageMask = _waitDstStageMasks.data(),
    .commandBufferCount = static_cast<uint32_t>(_commandBuffers.size()),
    .pCommandBuffers = _commandBuffers.data(),
    .signalSemaphoreCount = static_cast<uint32_t>(_signalSemaphores.size()),
    .pSignalSemaphores = _signalSemaphores.data()};
  return vkQueueSubmit(queue, 1, &submitInfo, fence);
}
