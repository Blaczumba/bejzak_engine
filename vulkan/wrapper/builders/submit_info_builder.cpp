#include "vulkan/wrapper/builders/submit_info_builder.h"

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vulkan/vulkan.h>

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withWaitSemaphores(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMasks) {
  assert(waitSemaphores.size() == waitDstStageMasks.size());
  _waitSemaphores = waitSemaphores;
  _waitDstStageMasks = waitDstStageMasks;
  _submitInfo.waitSemaphoreCount = static_cast<uint32_t>(_waitSemaphores.size());
  _submitInfo.pWaitSemaphores = _waitSemaphores.data();
  _submitInfo.pWaitDstStageMask = _waitDstStageMasks.data();
  return *this;
}

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withWaitSemaphores(
    std::initializer_list<VkSemaphore> waitSemaphores,
    std::initializer_list<VkPipelineStageFlags> waitDstStageMasks) {
  assert(waitSemaphores.size() == waitDstStageMasks.size());
  _waitSemaphores = lib::Buffer(waitSemaphores);
  _waitDstStageMasks = lib::Buffer(waitDstStageMasks);
  _submitInfo.waitSemaphoreCount = static_cast<uint32_t>(_waitSemaphores.size());
  _submitInfo.pWaitSemaphores = _waitSemaphores.data();
  _submitInfo.pWaitDstStageMask = _waitDstStageMasks.data();
  return *this;
}

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withSignalSemaphores(
    std::span<const VkSemaphore> signalSemaphores) {
  _signalSemaphores = signalSemaphores;
  _submitInfo.signalSemaphoreCount = static_cast<uint32_t>(_signalSemaphores.size());
  _submitInfo.pSignalSemaphores = _signalSemaphores.data();
  return *this;
}

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withSignalSemaphores(
    std::initializer_list<VkSemaphore> signalSemaphores) {
  _signalSemaphores = signalSemaphores;
  _submitInfo.signalSemaphoreCount = static_cast<uint32_t>(_signalSemaphores.size());
  _submitInfo.pSignalSemaphores = _signalSemaphores.data();
  return *this;
}

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withCommandBuffers(
    std::span<const VkCommandBuffer> commandBuffers) {
  _commandBuffers = commandBuffers;
  _submitInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
  _submitInfo.pCommandBuffers = _commandBuffers.data();
  return *this;
}

SubmitInfoOwningBuilder& SubmitInfoOwningBuilder::withCommandBuffers(
    std::initializer_list<VkCommandBuffer> commandBuffers) {
  _commandBuffers = commandBuffers;
  _submitInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
  _submitInfo.pCommandBuffers = _commandBuffers.data();
  return *this;
}

VkResult SubmitInfoOwningBuilder::submitQueue(VkQueue queue, VkFence fence) const noexcept {
  return vkQueueSubmit(queue, 1, &_submitInfo, fence);
}

SubmitInfoNonOwningBuilder& SubmitInfoNonOwningBuilder::withWaitSemaphores(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMasks) noexcept {
  assert(waitSemaphores.size() == waitDstStageMasks.size());
  _submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  _submitInfo.pWaitSemaphores = waitSemaphores.data();
  _submitInfo.pWaitDstStageMask = waitDstStageMasks.data();
  return *this;
}

SubmitInfoNonOwningBuilder& SubmitInfoNonOwningBuilder::withSignalSemaphores(
    std::span<const VkSemaphore> signalSemaphores) noexcept {
  _submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  _submitInfo.pSignalSemaphores = signalSemaphores.data();
  return *this;
}

SubmitInfoNonOwningBuilder& SubmitInfoNonOwningBuilder::withCommandBuffers(
    std::span<const VkCommandBuffer> commandBuffers) noexcept {
  _submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
  _submitInfo.pCommandBuffers = commandBuffers.data();
  return *this;
}

VkResult SubmitInfoNonOwningBuilder::submitQueue(VkQueue queue, VkFence fence) const noexcept {
  return vkQueueSubmit(queue, 1, &_submitInfo, fence);
}
