#pragma once

#include "vulkan_lib/logical_device/logical_device.h"
#include "vulkan_lib/render_pass/attachment_layout.h"
#include "vulkan_lib/status/status.h"

#include <memory>
#include <optional>
#include <span>
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

		Status addOutputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding);

		Status addInputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout);

		VkSubpassDescription getVkSubpassDescription() const;

		std::span<const VkAttachmentReference> getInputAttachmentRefs() const { return _inputAttachmentRefs; }

		std::span<const VkAttachmentReference> getColorAttachmentRefs() const { return _colorAttachmentRefs; }

		std::span<const VkAttachmentReference> getDepthAttachmentRefs() const { return _depthAttachmentRefs; }

		std::span<const VkAttachmentReference> getColorResolveAttachmentRefs() const { return _colorAttachmentResolveRefs; }
	};

public:
	// Allocates memory for object and leaves it in initial state.
	static std::unique_ptr<Renderpass> create(const LogicalDevice& logicalDevice, const AttachmentLayout& layout);

	~Renderpass();

	// Aggregates subpasses and dependencies and creates VkRenderPass object.
	Status build();

	VkRenderPass getVkRenderPass() const;

	const AttachmentLayout& getAttachmentsLayout() const;

	Status addSubpass(std::initializer_list<uint8_t> outputAttachments, std::initializer_list<uint8_t> inputAttachments = {});

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
