#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "common/status/status.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/render_pass/attachment_layout.h"

class Renderpass;

class RenderpassBuilder {
  class Subpass {
    std::vector<VkAttachmentReference> _inputAttachmentRefs;
    std::vector<VkAttachmentReference> _colorAttachmentRefs;
    std::vector<VkAttachmentReference> _depthAttachmentRefs;
    std::vector<VkAttachmentReference> _colorAttachmentResolveRefs;

  public:
    Subpass() = default;

    ~Subpass() = default;

    Status addOutputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding);

    Status addInputAttachment(
        const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout);

    VkSubpassDescription getVkSubpassDescription() const;
  };

public:
  RenderpassBuilder(const AttachmentLayout& attachmentLayout);

  RenderpassBuilder& addDependency(
      uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask,
      VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);

  RenderpassBuilder& addSubpass(std::initializer_list<uint8_t> outputAttachments,
                                std::initializer_list<uint8_t> inputAttachments = {});

  RenderpassBuilder& withMultiView(
      std::vector<uint32_t>&& viewMask, std::vector<uint32_t>&& correlationMask);

  ErrorOr<Renderpass> build(const LogicalDevice& logicalDevice);

private:
  const AttachmentLayout& _attachmentLayout;

  struct MultiViewInfo {
    VkRenderPassMultiviewCreateInfo multiviewCreateInfo;
    std::vector<uint32_t> viewMasks;
    std::vector<uint32_t> correlationMasks;
  };
  std::optional<MultiViewInfo> _multiViewInfo;

  std::vector<VkSubpassDependency> _subpassDepencies;
  std::vector<Subpass> _subpasses;

  Status _status;
};

class Renderpass {
public:
  Renderpass() = default;

  Renderpass(Renderpass&& renderpass) noexcept;

  Renderpass& operator=(Renderpass&& renderpass) noexcept;

  ~Renderpass();

  VkRenderPass getVkRenderPass() const;

  const AttachmentLayout& getAttachmentsLayout() const;

  const LogicalDevice& getLogicalDevice() const;

private:
  Renderpass(const LogicalDevice& logicalDeivce, VkRenderPass renderpass,
             const AttachmentLayout& attachmentLayout);

  VkRenderPass _renderpass = VK_NULL_HANDLE;

  const LogicalDevice* _logicalDevice = nullptr;
  AttachmentLayout _attachmentsLayout;

  friend class RenderpassBuilder;
};
