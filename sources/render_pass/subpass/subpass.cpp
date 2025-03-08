#include "subpass.h"

#include "render_pass/attachment/attachment_layout.h"

#include <stdexcept>

Subpass::Subpass(const AttachmentLayout& layout) : _layout(layout) {}

VkSubpassDescription Subpass::getVkSubpassDescription() const {
    return VkSubpassDescription {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = static_cast<uint32_t>(_inputAttachmentRefs.size()),
        .pInputAttachments = !_inputAttachmentRefs.empty() ? _inputAttachmentRefs.data() : nullptr,
        .colorAttachmentCount = static_cast<uint32_t>(_colorAttachmentRefs.size()),
        .pColorAttachments = !_colorAttachmentRefs.empty() ? _colorAttachmentRefs.data() : nullptr,
        .pResolveAttachments = !_colorAttachmentResolveRefs.empty() ? _colorAttachmentResolveRefs.data() : nullptr,
        .pDepthStencilAttachment = !_depthAttachmentRefs.empty() ? _depthAttachmentRefs.data() : nullptr
    };
}

lib::Status Subpass::addOutputAttachment(uint32_t attachmentBinding) {
    if (_layout.getAttachmentsCount() <= attachmentBinding)
        return lib::Error("attachmentBinding is not a valid index in attachments vector!");

    switch (_layout.getAttachmentsTypes()[attachmentBinding]) {
    case Attachment::Type::COLOR:
        _colorAttachmentRefs.emplace_back(attachmentBinding, _layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    case Attachment::Type::COLOR_RESOLVE:
        _colorAttachmentResolveRefs.emplace_back(attachmentBinding, _layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    case Attachment::Type::DEPTH:
        _depthAttachmentRefs.emplace_back(attachmentBinding, _layout.getVkSubpassLayouts()[attachmentBinding]);
        break;
    default:
        return lib::Error("Unknown attachment type");
    }
    return lib::StatusOk();
}

lib::Status Subpass::addInputAttachment(uint32_t attachmentBinding, VkImageLayout imageLayout) {
    if (_layout.getAttachmentsCount() <= attachmentBinding)
        return lib::Error("attachmentBinding is not a valid index in attachments vector!");
    _inputAttachmentRefs.emplace_back(attachmentBinding, imageLayout);
    return lib::StatusOk();
}
