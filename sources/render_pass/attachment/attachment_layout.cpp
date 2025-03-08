#include "attachment_layout.h"

#include <algorithm>
#include <iterator>

AttachmentLayout::AttachmentLayout(VkSampleCountFlagBits numMsaaSamples) : _numMsaaSamples(numMsaaSamples) {}

VkSampleCountFlagBits AttachmentLayout::getNumMsaaSamples() const {
	return _numMsaaSamples;
}

const std::vector<VkClearValue>& AttachmentLayout::getVkClearValues() const {
	return _clearValues;
}

const std::vector<VkAttachmentDescription>& AttachmentLayout::getVkAttachmentDescriptions() const {
	return _attachmentDescriptions;
}

const std::vector<VkImageLayout>& AttachmentLayout::getVkSubpassLayouts() const {
	return _subpassImageLayouts;
}

const std::vector<Attachment::Type>& AttachmentLayout::getAttachmentsTypes() const {
	return _attachmentTypes;
}

uint32_t AttachmentLayout::getAttachmentsCount() const {
	return _attachmentTypes.size();
}

uint32_t AttachmentLayout::getColorAttachmentsCount() const {
	return std::count(_attachmentTypes.cbegin(), _attachmentTypes.cend(), Attachment::Type::COLOR);
}

AttachmentLayout& AttachmentLayout::addColorAttachment(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
	_clearValues.emplace_back(VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } });
	_attachmentDescriptions.emplace_back(createDescription(format, _numMsaaSamples, loadOp, storeOp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::COLOR);
	return *this;
}

AttachmentLayout& AttachmentLayout::addColorPresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp) {
	_clearValues.emplace_back(VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } });
	_attachmentDescriptions.emplace_back(createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::COLOR);
	return *this;
}

AttachmentLayout& AttachmentLayout::addDepthAttachment(VkFormat format, VkAttachmentStoreOp storeOp, VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp) {
	_clearValues.emplace_back(VkClearValue{ .depthStencil = { 1.0f, 0 } });
	_attachmentDescriptions.emplace_back(createDescription(format, _numMsaaSamples, VK_ATTACHMENT_LOAD_OP_CLEAR, storeOp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, stencilLoadOp, stencilStoreOp));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::DEPTH);
	return *this;
}

AttachmentLayout& AttachmentLayout::addShadowAttachment(VkFormat format, VkImageLayout finalLayout) {
	_clearValues.emplace_back(VkClearValue{ .depthStencil = { 1.0f, 0 } });
	_attachmentDescriptions.emplace_back(createDescription(format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::DEPTH);
	return *this;
}

AttachmentLayout& AttachmentLayout::addColorResolveAttachment(VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
	_clearValues.emplace_back(VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } });
	_attachmentDescriptions.emplace_back(createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, storeOp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::COLOR_RESOLVE);
	return *this;
}

AttachmentLayout& AttachmentLayout::addColorResolvePresentAttachment(VkFormat format, VkAttachmentLoadOp loadOp) {
	_clearValues.emplace_back(VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 1.0f } });
	_attachmentDescriptions.emplace_back(createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
	_subpassImageLayouts.emplace_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	_attachmentTypes.emplace_back(Attachment::Type::COLOR_RESOLVE);
	return *this;
}

VkAttachmentDescription AttachmentLayout::createDescription(
	VkFormat format,
	VkSampleCountFlagBits samples,
	VkAttachmentLoadOp loadOp,
	VkAttachmentStoreOp storeOp,
	VkImageLayout initialLayout,
	VkImageLayout finalLayout,
	VkAttachmentLoadOp stencilLoadOp,
	VkAttachmentStoreOp stencilStoreOp) {
	return VkAttachmentDescription{
		.flags = 0,
		.format = format,
		.samples = samples,
		.loadOp = loadOp,
		.storeOp = storeOp,
		.stencilLoadOp = stencilLoadOp,
		.stencilStoreOp = stencilStoreOp,
		.initialLayout = initialLayout,
		.finalLayout = finalLayout
	};
}
