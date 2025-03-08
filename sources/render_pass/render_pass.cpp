#include "render_pass.h"

#include "render_pass/attachment/attachment_layout.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

Renderpass::Renderpass(const LogicalDevice& logicalDevice, const AttachmentLayout& layout) 
    : _logicalDevice(logicalDevice), _attachmentsLayout(layout) {
}

lib::ErrorOr<std::unique_ptr<Renderpass>> Renderpass::create(const LogicalDevice& logicalDevice, const AttachmentLayout& layout) {
    return std::unique_ptr<Renderpass>(new Renderpass(logicalDevice, layout));
}

lib::Status Renderpass::build() {
    if (_renderpass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_logicalDevice.getVkDevice(), _renderpass, nullptr);
    }

    const std::vector<VkAttachmentDescription>& attachmentDescriptions = _attachmentsLayout.getVkAttachmentDescriptions();
    lib::Buffer<VkSubpassDescription> subpassDescriptions(_subpasses.size());
    std::transform(_subpasses.cbegin(), _subpasses.cend(), subpassDescriptions.begin(), [](const Subpass& subpass) { return subpass.getVkSubpassDescription(); });

    const VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
        .pSubpasses = subpassDescriptions.data(),
        .dependencyCount = static_cast<uint32_t>(_subpassDepencies.size()),
        .pDependencies = _subpassDepencies.data()
    };

    if (vkCreateRenderPass(_logicalDevice.getVkDevice(), &renderPassInfo, nullptr, &_renderpass) != VK_SUCCESS) {
        return lib::Error("failed to create render pass!");
    }

    return lib::StatusOk();
}

Renderpass::~Renderpass() {
    vkDestroyRenderPass(_logicalDevice.getVkDevice(), _renderpass, nullptr);
}

const VkRenderPass Renderpass::getVkRenderPass() const {
    return _renderpass;
}

const AttachmentLayout& Renderpass::getAttachmentsLayout() const {
    return _attachmentsLayout;
}

void Renderpass::addSubpass(const Subpass& subpass) {
    _subpasses.push_back(subpass);
}

void Renderpass::addDependency(uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
    _subpassDepencies.emplace_back(srcSubpassIndex, dstSubpassIndex, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask);
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
    return _logicalDevice;
}
