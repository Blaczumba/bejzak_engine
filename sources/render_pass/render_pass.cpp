#include "render_pass.h"

#include "lib/macros/status_macros.h"
#include "render_pass/attachment_layout.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

Status Renderpass::Subpass::addOutputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding) {
    if (layout.getAttachmentsCount() <= attachmentBinding) {
        return Error(EngineError::INDEX_OUT_OF_RANGE);
    }

    switch (layout.getAttachmentsTypes()[attachmentBinding]) {
    case Attachment::Type::COLOR:
        _colorAttachmentRefs.emplace_back(attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    case Attachment::Type::COLOR_RESOLVE:
        _colorAttachmentResolveRefs.emplace_back(attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    case Attachment::Type::DEPTH:
        _depthAttachmentRefs.emplace_back(attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    default:
        return Error(EngineError::NOT_RECOGNIZED_TYPE);
    }
    return StatusOk();
}

Status Renderpass::Subpass::addInputAttachment(const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout) {
    if (layout.getAttachmentsCount() <= attachmentBinding) {
        return Error(EngineError::INDEX_OUT_OF_RANGE);
    }
    _inputAttachmentRefs.emplace_back(attachmentBinding, imageLayout);
    return StatusOk();
}

VkSubpassDescription Renderpass::Subpass::getVkSubpassDescription() const {
    return VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = static_cast<uint32_t>(_inputAttachmentRefs.size()),
        .pInputAttachments = !_inputAttachmentRefs.empty() ? _inputAttachmentRefs.data() : nullptr,
        .colorAttachmentCount = static_cast<uint32_t>(_colorAttachmentRefs.size()),
        .pColorAttachments = !_colorAttachmentRefs.empty() ? _colorAttachmentRefs.data() : nullptr,
        .pResolveAttachments = !_colorAttachmentResolveRefs.empty() ? _colorAttachmentResolveRefs.data() : nullptr,
        .pDepthStencilAttachment = !_depthAttachmentRefs.empty() ? _depthAttachmentRefs.data() : nullptr
    };
}

Renderpass::Renderpass(const LogicalDevice& logicalDevice, const AttachmentLayout& layout) 
    : _logicalDevice(logicalDevice), _attachmentsLayout(layout) {
}

std::unique_ptr<Renderpass> Renderpass::create(const LogicalDevice& logicalDevice, const AttachmentLayout& layout) {
    return std::unique_ptr<Renderpass>(new Renderpass(logicalDevice, layout));
}

Status Renderpass::build() {
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

    if (VkResult result = vkCreateRenderPass(_logicalDevice.getVkDevice(), &renderPassInfo, nullptr, &_renderpass); result != VK_SUCCESS) {
        return Error(result);
    }

    return StatusOk();
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

Status Renderpass::addSubpass(std::initializer_list<uint8_t> outputAttachments, std::initializer_list<uint8_t> inputAttachments) {
    Subpass subpass;
    for (uint8_t index : outputAttachments) {
        RETURN_IF_ERROR(subpass.addOutputAttachment(_attachmentsLayout, index));
    }
    for (uint8_t index : inputAttachments) {
        // TODO set proper image layouts
        RETURN_IF_ERROR(subpass.addInputAttachment(_attachmentsLayout, index, VK_IMAGE_LAYOUT_GENERAL));
    }
    _subpasses.push_back(subpass);
    return StatusOk();
}

void Renderpass::addDependency(uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
    _subpassDepencies.emplace_back(srcSubpassIndex, dstSubpassIndex, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask);
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
    return _logicalDevice;
}
