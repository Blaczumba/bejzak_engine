#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Attachment {

enum class Type : uint8_t {
	COLOR = 0,
	COLOR_RESOLVE,
	DEPTH
};

}

class AttachmentLayout {
public:
	const std::vector<VkClearValue>& getVkClearValues() const;
	const std::vector<VkAttachmentDescription>& getVkAttachmentDescriptions() const;
	const std::vector<VkImageLayout>& getVkSubpassLayouts() const;
	const std::vector<Attachment::Type>& getAttachmentsTypes() const;

	uint32_t getAttachmentsCount() const;
	uint32_t getColorAttachmentsCount() const;

	void addColorAttachment(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
	void addColorPresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp);
	void addDepthAttachment(VkFormat format, VkAttachmentStoreOp storeOp, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
	void addShadowAttachment(VkFormat format, VkImageLayout finalLayout);
	void addColorResolveAttachment(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);
	void addColorResolvePresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp);

private:
	std::vector<VkClearValue> _clearValues;
	std::vector<VkAttachmentDescription> _attachmentDescriptions;
	std::vector<VkImageLayout> _subpassImageLayouts;
	std::vector<Attachment::Type> _attachmentTypes;

	VkAttachmentDescription createDescription(
		VkFormat format,
		VkSampleCountFlagBits samples,
		VkAttachmentLoadOp loadOp,
		VkAttachmentStoreOp storeOp,
		VkImageLayout initialLayout,
		VkImageLayout finalLayout,
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
};
