#pragma once

#include <span>
#include <vector>
#include <vulkan/vulkan.h>

enum class AttachmentType : uint8_t {
  COLOR = 0,
  COLOR_RESOLVE,
  DEPTH
};

class AttachmentLayout {
public:
  explicit AttachmentLayout(VkSampleCountFlagBits numMsaaSamples = VK_SAMPLE_COUNT_1_BIT);

  std::span<const VkClearValue> getVkClearValues() const;
  std::span<const VkAttachmentDescription> getVkAttachmentDescriptions() const;
  std::span<const VkImageLayout> getVkSubpassLayouts() const;
  std::span<const AttachmentType> getAttachmentsTypes() const;

  uint32_t getColorAttachmentsCount() const;

  VkSampleCountFlagBits getNumMsaaSamples() const;
  AttachmentLayout& addColorAttachment(
      VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
  AttachmentLayout& addColorPresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp);
  AttachmentLayout& addDepthAttachment(
      VkFormat format, VkAttachmentStoreOp storeOp,
      VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
  AttachmentLayout& addShadowAttachment(VkFormat format, VkImageLayout finalLayout);
  AttachmentLayout& addColorResolveAttachment(
      VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
  AttachmentLayout& addColorResolvePresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp);

private:
  VkSampleCountFlagBits _numMsaaSamples;
  std::vector<VkClearValue> _clearValues;
  std::vector<VkAttachmentDescription> _attachmentDescriptions;
  std::vector<VkImageLayout> _subpassImageLayouts;
  std::vector<AttachmentType> _attachmentTypes;
};
