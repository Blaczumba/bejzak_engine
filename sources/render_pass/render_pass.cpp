#include "render_pass.h"

#include "render_pass/attachment/attachment_layout.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

Renderpass::Renderpass(const LogicalDevice& logicalDevice, const AttachmentLayout& layout) 
    : _logicalDevice(logicalDevice), _attachmentsLayout(layout) {
}

void Renderpass::create() {
    cleanup();

    const std::vector<VkAttachmentDescription>& attachmentDescriptions = _attachmentsLayout.getVkAttachmentDescriptions();
    std::vector<VkSubpassDescription> subpassDescriptions;
    subpassDescriptions.reserve(_subpasses.size());
    std::transform(_subpasses.cbegin(), _subpasses.cend(), std::back_inserter(subpassDescriptions), [](const Subpass& subpass) { return subpass.getVkSubpassDescription(); });

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
        throw std::runtime_error("failed to create render pass!");
    }
}

void Renderpass::cleanup() {
    if (_renderpass != VK_NULL_HANDLE)
        vkDestroyRenderPass(_logicalDevice.getVkDevice(), _renderpass, nullptr);

    _renderpass = VK_NULL_HANDLE;
}

Renderpass::~Renderpass() {
    cleanup();
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
    const VkSubpassDependency dependency = {
        .srcSubpass = srcSubpassIndex,
        .dstSubpass = dstSubpassIndex,
        .srcStageMask = srcStageMask,
        .dstStageMask = dstStageMask,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask
    };
    _subpassDepencies.push_back(dependency);
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
    return _logicalDevice;
}
