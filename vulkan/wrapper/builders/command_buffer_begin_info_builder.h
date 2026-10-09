#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

class CommandBufferBeginInfoNonOwningBuilder {
public:
  CommandBufferBeginInfoNonOwningBuilder() noexcept = default;

  ~CommandBufferBeginInfoNonOwningBuilder() = default;

  CommandBufferBeginInfoNonOwningBuilder(const CommandBufferBeginInfoNonOwningBuilder&) = delete;

  CommandBufferBeginInfoNonOwningBuilder& operator=(
      const CommandBufferBeginInfoNonOwningBuilder&) = delete;

  CommandBufferBeginInfoNonOwningBuilder& withViewportScissorInheritenceInfo(
      std::span<const VkViewport> viewports) noexcept;

  CommandBufferBeginInfoNonOwningBuilder& withInheritenceInfo(
      VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t subpass,
      std::optional<VkQueryControlFlags> queryControlFlags = std::nullopt,
      VkQueryPipelineStatisticFlags pipelineStatistics = {}) noexcept;

  CommandBufferBeginInfoNonOwningBuilder& withFlags(VkCommandBufferUsageFlags flags) noexcept;

  const VkCommandBufferBeginInfo& build() const noexcept;

private:
  std::optional<VkCommandBufferInheritanceViewportScissorInfoNV> _viewportScissorInheritanceInfo;
  std::optional<VkCommandBufferInheritanceInfo> _inheritanceInfo;

  VkCommandBufferBeginInfo _beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
};
