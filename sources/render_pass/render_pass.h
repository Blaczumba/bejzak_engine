#pragma once

#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "memory_objects/texture/texture_factory.h"
#include "render_pass/attachment/attachment_layout.h"

#include <memory>
#include <optional>
#include <vector>

class CommandPool;
class LogicalDevice;

class Renderpass {
	class Subpass {
		std::vector<VkAttachmentReference> _inputAttachmentRefs;
		std::vector<VkAttachmentReference> _colorAttachmentRefs;
		std::vector<VkAttachmentReference> _depthAttachmentRefs;
		std::vector<VkAttachmentReference> _colorAttachmentResolveRefs;

	public:
		Subpass() = default;

		~Subpass() = default;

		lib::Status addOutputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding);
		lib::Status addInputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout);
		VkSubpassDescription getVkSubpassDescription() const;

		const std::vector<VkAttachmentReference>& getInputAttachmentRefs() const { return _inputAttachmentRefs; }
		const std::vector<VkAttachmentReference>& getColorAttachmentRefs() const { return _colorAttachmentRefs; }
		const std::vector<VkAttachmentReference>& getDepthAttachmentRefs() const { return _depthAttachmentRefs; }
		const std::vector<VkAttachmentReference>& getColorResolveAttachmentRefs() const { return _colorAttachmentResolveRefs; }
	};

public:
	// Allocates memory for object and leaves it in initial state.
	static lib::ErrorOr<std::unique_ptr<Renderpass>> create(const LogicalDevice& logicalDevice, const AttachmentLayout& layout);

	~Renderpass();

	// Aggregates subpasses and dependencies and creates VkRenderPass object.
	lib::Status build();

	const VkRenderPass getVkRenderPass() const;
	const AttachmentLayout& getAttachmentsLayout() const;

	lib::Status addSubpass(std::initializer_list<uint8_t> outputAttachments, std::initializer_list<uint8_t> inputAttachments = {});
	void addDependency(uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask);

	const LogicalDevice& getLogicalDevice() const;

private:
	Renderpass(const LogicalDevice& logicalDevice, const AttachmentLayout& layout);

	VkRenderPass _renderpass = VK_NULL_HANDLE;
	AttachmentLayout _attachmentsLayout;

	std::vector<Subpass> _subpasses;
	std::vector<VkSubpassDependency> _subpassDepencies;

	const LogicalDevice& _logicalDevice;
};
