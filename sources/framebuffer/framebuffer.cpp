#include "framebuffer.h"

#include "command_buffer/command_buffer.h"
#include "lib/buffer/buffer.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "swapchain/swapchain.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromSwapchain(const VkCommandBuffer commandBuffer, const Renderpass& renderpass, const Swapchain& swapchain, uint8_t swapchainImageIndex) {
    if (swapchain.getImagesCount() <= swapchainImageIndex) {
        return Error(EngineError::INDEX_OUT_OF_RANGE);
    }
    const VkExtent2D swapchainExtent = swapchain.getExtent();
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
    const std::vector<VkAttachmentDescription>& descriptions = renderpass.getAttachmentsLayout().getVkAttachmentDescriptions();

    lib::Buffer<VkImageView> imageViews(descriptions.size());
    lib::Buffer<std::shared_ptr<Texture>> textureAttachments(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i) {
        const auto& description = descriptions[i];
        switch (description.finalLayout) {
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            imageViews[i] = swapchain.getVkImageViews()[swapchainImageIndex];
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            ASSIGN_OR_RETURN(textureAttachments[i], Texture::createColorAttachment(logicalDevice, commandBuffer, description.format, description.samples, swapchainExtent));
            imageViews[i] = textureAttachments[i]->getVkImageView();
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            ASSIGN_OR_RETURN(textureAttachments[i], Texture::createDepthAttachment(logicalDevice, commandBuffer, description.format, description.samples, swapchainExtent));
            imageViews[i] = textureAttachments[i]->getVkImageView();
            break;
        }
        default:
            return Error(EngineError::NOT_SUPPORTED_FRAMEBUFFER_IMAGE_LAYOUT);
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
    return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor, std::move(textureAttachments)));
}

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromTextures(const Renderpass& renderpass, lib::Buffer<std::shared_ptr<Texture>>&& textures) {
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
    return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor, std::move(textures)));
}

Framebuffer::Framebuffer(const VkFramebuffer framebuffer, const Renderpass& renderpass, const VkViewport& viewport, const VkRect2D& scissor, lib::Buffer<std::shared_ptr<Texture>>&& textures)
    : _framebuffer(framebuffer), _renderpass(renderpass), _viewport(viewport), _scissor(scissor), _textureAttachments(std::move(textures)) {}

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
