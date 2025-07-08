#include "framebuffer.h"

#include "command_buffer/command_buffer.h"
#include "lib/buffer/buffer.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "swapchain/swapchain.h"

#include <algorithm>
#include <iterator>
#include <optional>

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromSwapchain(const VkCommandBuffer commandBuffer, const Renderpass& renderpass, VkExtent2D swapchainExtent, const VkImageView swapchainImageView, std::vector<std::unique_ptr<Texture>>& attachments) {
    const VkViewport viewport = {
        .width = static_cast<float>(swapchainExtent.width),
        .height = static_cast<float>(swapchainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    const VkRect2D scissor = {
        .extent = swapchainExtent
    };

    const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
    std::span<const VkAttachmentDescription> descriptions = renderpass.getAttachmentsLayout().getVkAttachmentDescriptions();
    lib::Buffer<VkImageView> imageViews(descriptions.size());

    for (size_t i = 0; i < imageViews.size(); ++i) {
        const auto& description = descriptions[i];
        switch (description.finalLayout) {
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            imageViews[i] = swapchainImageView;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            ASSIGN_OR_RETURN(std::unique_ptr<Texture> attachment, Texture::createColorAttachment(logicalDevice, commandBuffer, description.format, description.samples, swapchainExtent));
            imageViews[i] = attachment->getVkImageView();
            attachments.push_back(std::move(attachment));
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            ASSIGN_OR_RETURN(std::unique_ptr<Texture> attachment, Texture::createDepthAttachment(logicalDevice, commandBuffer, description.format, description.samples, swapchainExtent));
            imageViews[i] = attachment->getVkImageView();
            attachments.push_back(std::move(attachment));
            break;
        }
        default:
            return Error(EngineError::NOT_RECOGNIZED_TYPE);
        }
    }

    const VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderpass.getVkRenderPass(),
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = swapchainExtent.width,
        .height = swapchainExtent.height,
        .layers = 1,
    };

    VkFramebuffer framebuffer;
    if (VkResult result = vkCreateFramebuffer(renderpass.getLogicalDevice().getVkDevice(), &framebufferInfo, nullptr, &framebuffer); result != VK_SUCCESS) {
        return Error(result);
    }
    return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor));
}

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromTextures(const Renderpass& renderpass, std::span<const std::unique_ptr<Texture>> textures) {
    lib::Buffer<VkImageView> imageViews(textures.size());
    std::optional<VkExtent2D> extent;
    for (size_t i = 0; i < textures.size(); ++i) {
        const auto& texture = *textures[i];
        imageViews[i] = texture.getVkImageView();
        if (!extent.has_value()) {
            extent = texture.getVkExtent2D();
        } else if (VkExtent2D tmpExtent = texture.getVkExtent2D(); extent->width != tmpExtent.width || extent->height != tmpExtent.height) {
            return Error(EngineError::SIZE_MISMATCH);
        }
    }

    if (!extent.has_value()) {
        return Error(EngineError::EMPTY_COLLECTION);
    }

    const VkViewport viewport = {
        .width = static_cast<float>(extent->width),
        .height = static_cast<float>(extent->height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const VkRect2D scissor = {
        .extent = *extent
    };

    const VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderpass.getVkRenderPass(),
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = extent->width,
        .height = extent->height,
        .layers = 1,
    };

    VkFramebuffer framebuffer;
    if (VkResult result = vkCreateFramebuffer(renderpass.getLogicalDevice().getVkDevice(), &framebufferInfo, nullptr, &framebuffer); result != VK_SUCCESS) {
        return Error(result);
    }
    return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor));
}

Framebuffer::Framebuffer(const VkFramebuffer framebuffer, const Renderpass& renderpass, const VkViewport& viewport, const VkRect2D& scissor)
    : _framebuffer(framebuffer), _renderpass(renderpass), _viewport(viewport), _scissor(scissor) {}

Framebuffer::~Framebuffer() {
    vkDestroyFramebuffer(_renderpass.getLogicalDevice().getVkDevice(), _framebuffer, nullptr);
}

VkExtent2D Framebuffer::getVkExtent() const {
    return _scissor.extent;
}

const VkViewport& Framebuffer::getViewport() const {
    return _viewport;
}

const VkRect2D& Framebuffer::getScissor() const {
    return _scissor;
}

const Renderpass& Framebuffer::getRenderpass() const {
    return _renderpass;
}

VkFramebuffer Framebuffer::getVkFramebuffer() const {
    return _framebuffer;
}
