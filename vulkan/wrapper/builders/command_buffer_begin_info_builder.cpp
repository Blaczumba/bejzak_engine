#include "vulkan/wrapper/builders/command_buffer_begin_info_builder.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

namespace {

template <typename T>
void chainExtendedField(const void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

CommandBufferBeginInfoNonOwningBuilder& CommandBufferBeginInfoNonOwningBuilder::
    withViewportScissorInheritenceInfo(std::span<const VkViewport> viewports) noexcept {
  assert(_inheritanceInfo.has_value());  // The feature can be enabled after the inheritence is
                                         // included.
  assert(!_viewportScissorInheritanceInfo.has_value());
  _viewportScissorInheritanceInfo = VkCommandBufferInheritanceViewportScissorInfoNV{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV,
    .viewportScissor2D = VK_TRUE,
    .viewportDepthCount = static_cast<uint32_t>(viewports.size()),
    .pViewportDepths = viewports.data()};
  chainExtendedField(&_inheritanceInfo->pNext, *_viewportScissorInheritanceInfo);
  return *this;
}

CommandBufferBeginInfoNonOwningBuilder& CommandBufferBeginInfoNonOwningBuilder::withInheritenceInfo(
    VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t subpass,
    std::optional<VkQueryControlFlags> queryControlFlags,
    VkQueryPipelineStatisticFlags pipelineStatistics) noexcept {
  assert(!_inheritanceInfo.has_value());
  _inheritanceInfo = VkCommandBufferInheritanceInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
    .renderPass = renderpass,
    .subpass = subpass,
    .framebuffer = framebuffer,
    .occlusionQueryEnable = queryControlFlags.has_value() ? VK_TRUE : VK_FALSE,
    .queryFlags = queryControlFlags.value_or(0),
    .pipelineStatistics = pipelineStatistics};
  _beginInfo.pInheritanceInfo = &_inheritanceInfo.value();
  return *this;
}

CommandBufferBeginInfoNonOwningBuilder& CommandBufferBeginInfoNonOwningBuilder::withFlags(
    VkCommandBufferUsageFlags flags) noexcept {
  _beginInfo.flags = flags;
  return *this;
}

const VkCommandBufferBeginInfo& CommandBufferBeginInfoNonOwningBuilder::build() const noexcept {
  return _beginInfo;
}
