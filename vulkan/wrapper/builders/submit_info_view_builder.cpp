#include "vulkan/wrapper/builders/submit_info_view_builder.h"

SubmitInfoViewBuilder& SubmitInfoViewBuilder::withWaitSemaphores(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMasks) noexcept {
  _submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  _submitInfo.pWaitSemaphores = waitSemaphores.data();
  _submitInfo.pWaitDstStageMask = waitDstStageMasks.data();
  return *this;
}

SubmitInfoViewBuilder& SubmitInfoViewBuilder::withSignalSemaphores(
    std::span<const VkSemaphore> signalSemaphores) noexcept {
  _submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  _submitInfo.pSignalSemaphores = signalSemaphores.data();
  return *this;
}

SubmitInfoViewBuilder& SubmitInfoViewBuilder::withCommandBuffers(
    std::span<const VkCommandBuffer> commandBuffers) noexcept {
  _submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
  _submitInfo.pCommandBuffers = commandBuffers.data();
  return *this;
}

VkResult SubmitInfoViewBuilder::submitQueue(VkQueue queue, VkFence fence) noexcept {
  _submitInfo.pNext = _pNext;
  return vkQueueSubmit(queue, 1, &_submitInfo, fence);
}
