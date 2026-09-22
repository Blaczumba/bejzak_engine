#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/command_buffer/command_pool.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

class CommandBuffer {
  CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool,
                VkCommandBuffer commandBuffer, VkCommandBufferLevel level) noexcept;

public:
  CommandBuffer() noexcept = default;

  static CommandBuffer create(
      const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level);

  template <size_t COUNT>
  static std::array<CommandBuffer, COUNT> create(
      const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level);

  static std::vector<CommandBuffer> create(const std::shared_ptr<const CommandPool>& commandPool,
                                           VkCommandBufferLevel level, uint32_t count);

  CommandBuffer(CommandBuffer&&) noexcept;

  CommandBuffer& operator=(CommandBuffer&&) noexcept;

  ~CommandBuffer();

  void beginRenderPass(
      VkSubpassContents subpassContents, VkFramebuffer framebuffer, VkExtent2D framebufferExtent,
      VkRenderPass renderpass, std::span<const VkClearValue> clearValues) const;

  void endRenderPass() const;

  void setVieport(std::span<const VkViewport> viewports, uint32_t firstVieport = 0) const noexcept;

  void setVieport(
      std::initializer_list<VkViewport> viewports, uint32_t firstVieport = 0) const noexcept;

  void setScissor(std::span<const VkRect2D> scissors, uint32_t firstScissor = 0) const noexcept;

  void setScissor(
      std::initializer_list<VkRect2D> scissors, uint32_t firstScissor = 0) const noexcept;

  void bindVertexBuffers(std::span<const VkBuffer> buffers, std::span<const VkDeviceSize> offsets,
                         uint32_t firstBinding = 0) const noexcept;

  void bindVertexBuffers(
      std::initializer_list<VkBuffer> buffers, std::initializer_list<VkDeviceSize> offsets,
      uint32_t firstBinding = 0) const noexcept;

  void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const noexcept;

  void pipelineBarrier(const VkDependencyInfo* dependencyInfo) const noexcept;

  void bindIndexBuffer(
      VkBuffer buffer, VkIndexType indexType, VkDeviceSize offset = 0) const noexcept;

  void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                          std::span<const VkDescriptorSet> descriptorSets, uint32_t firstSet = 0,
                          std::span<const uint32_t> dynamicOffsets = {}) const noexcept;

  void bindDescriptorSets(
      VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
      std::initializer_list<VkDescriptorSet> descriptorSets, uint32_t firstSet = 0,
      std::initializer_list<uint32_t> dynamicOffsets = {}) const noexcept;

  void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                     std::span<const std::byte> data, uint32_t offset = 0) const noexcept;

  void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0,
                   int32_t vertexOffset = 0, uint32_t firstInstance = 0) const noexcept;

  void executeSecondaryCommandBuffers(std::span<const VkCommandBuffer> commandBuffers) const;

  void dispatchCompute(
      uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ = 1) const noexcept;

  void transitionImageLayout(VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout,
                             VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount,
                             uint32_t baseArraylayer, uint32_t layerCount) const noexcept;

  void generateMipmaps(
      VkImage image, VkFormat imageFormat, VkImageLayout finalLayout, int32_t texWidth,
      int32_t texHeight, uint32_t mipLevels, uint32_t layerCount) const noexcept;

  void copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                          std::span<const VkBufferCopy> copyRegions) const noexcept;

  void copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                          std::initializer_list<VkBufferCopy> copyRegions) const noexcept;

  void copyBufferToImage(VkBuffer buffer, VkImage image,
                         std::span<const VkBufferImageCopy> copyRegions) const noexcept;

  void copyBufferToImage(VkBuffer buffer, VkImage image, VkImageLayout layout,
                         std::initializer_list<VkBufferImageCopy> copyRegions) const noexcept;

  class BeginInfoBuilder {
  public:
    BeginInfoBuilder() noexcept = default;

    ~BeginInfoBuilder() = default;

    BeginInfoBuilder& withViewportScissorInheritenceInfo(std::span<const VkViewport> viewports);

    BeginInfoBuilder& withInheritenceInfo(
        VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t subpass,
        std::optional<VkQueryControlFlags> queryControlFlags = std::nullopt,
        VkQueryPipelineStatisticFlags pipelineStatistics = 0);

    VkResult beginCommandBuffer(
        const CommandBuffer& commandBuffer, VkCommandBufferUsageFlags usageFlags = 0);

  private:
    void* _inheritenceInfoPNext = nullptr;
    std::optional<VkCommandBufferInheritanceViewportScissorInfoNV> _viewportScissorInheritanceInfo;
    std::optional<VkCommandBufferInheritanceInfo> _inheritanceInfo;

    void* _pNext = nullptr;
  };

  VkResult end() const;

  VkResult resetCommandBuffer(VkCommandBufferResetFlags flags = 0) const;

  VkCommandBuffer getVkCommandBuffer() const noexcept;

protected:
  VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
  std::shared_ptr<const CommandPool> _commandPool;

  VkCommandBufferLevel _level;

  void destroy();
};

namespace internal {

inline VkResult allocateCommandBuffers(
    VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level,
    std::span<VkCommandBuffer> outCommandBuffers) {
  const VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = level,
    .commandBufferCount = static_cast<uint32_t>(outCommandBuffers.size()),
  };
  return vkAllocateCommandBuffers(device, &allocInfo, outCommandBuffers.data());
}

}  // namespace internal

template <size_t COUNT>
std::array<CommandBuffer, COUNT> CommandBuffer::create(
    const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level) {
  VkCommandBuffer vkCommandBuffers[COUNT];
  CHECK_VKCMD(
      internal::allocateCommandBuffers(commandPool->getLogicalDevice().getVkDevice(),
                                       commandPool->getVkCommandPool(), level, vkCommandBuffers),
      "Failed to create VkCommandBuffer.");

  std::array<CommandBuffer, COUNT> commandBuffers;
  std::transform(std::cbegin(vkCommandBuffers), std::cend(vkCommandBuffers), commandBuffers.begin(),
                 [&commandPool, level](VkCommandBuffer commandBuffer) {
                   return CommandBuffer(commandPool, commandBuffer, level);
                 });
  return commandBuffers;
}

// Definition of the template declared in command_pool.h. It has to be defined here,
// after CommandBuffer is a complete type.
template <size_t COUNT>
std::array<CommandBuffer, COUNT> CommandPool::createCommandBuffers(
    VkCommandBufferLevel level) const {
  return CommandBuffer::create<COUNT>(shared_from_this(), level);
}
