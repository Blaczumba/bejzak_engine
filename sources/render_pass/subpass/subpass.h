#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <iterator>
#include <vector>

class AttachmentLayout;

class Subpass {
	const AttachmentLayout& _layout;

	std::vector<VkAttachmentReference> _inputAttachmentRefs;
	std::vector<VkAttachmentReference> _colorAttachmentRefs;
	std::vector<VkAttachmentReference> _depthAttachmentRefs;
	std::vector<VkAttachmentReference> _colorAttachmentResolveRefs;

public:
	Subpass(const AttachmentLayout& layout);
	void addSubpassOutputAttachment(uint32_t attachmentBinding);
	void addSubpassInputAttachment(uint32_t attachmentBinding, VkImageLayout layout);
	VkSubpassDescription getVkSubpassDescription() const;

	const std::vector<VkAttachmentReference>& getInputAttachmentRefs() const { return _inputAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getColorAttachmentRefs() const { return _colorAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getDepthAttachmentRefs() const { return _depthAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getColorResolveAttachmentRefs() const { return _colorAttachmentResolveRefs; }
};
