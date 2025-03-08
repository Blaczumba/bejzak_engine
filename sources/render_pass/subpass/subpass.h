#pragma once

#include "lib/status/status.h"

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
	explicit Subpass(const AttachmentLayout& layout);

	~Subpass() = default;

	lib::Status addOutputAttachment(uint32_t attachmentBinding);
	lib::Status addInputAttachment(uint32_t attachmentBinding, VkImageLayout imageLayout);
	VkSubpassDescription getVkSubpassDescription() const;

	const std::vector<VkAttachmentReference>& getInputAttachmentRefs() const { return _inputAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getColorAttachmentRefs() const { return _colorAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getDepthAttachmentRefs() const { return _depthAttachmentRefs; }
	const std::vector<VkAttachmentReference>& getColorResolveAttachmentRefs() const { return _colorAttachmentResolveRefs; }
};
