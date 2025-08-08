#include "framebuffer.h"

#include <optional>

#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/util/check.h"

namespace {

ErrorOr<Texture> createColorAttachment(
    const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkFormat format,
    VkSampleCountFlagBits samples, VkExtent2D extent) {
  return TextureBuilder()
      .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
      .withExtent(extent.width, extent.height)
      .withFormat(format)
      .withNumSamples(samples)
      .withUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
      .withLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
      .buildAttachment(logicalDevice, commandBuffer);
}

bool hasStencil(VkFormat format) {
  static constexpr VkFormat formats[] = {VK_FORMAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
                                         VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT};
  return std::find(std::cbegin(formats), std::cend(formats), format) != std::cend(formats);
}

ErrorOr<Texture> createDepthAttachment(
    const LogicalDevice& logicalDevice, VkCommandBuffer commandBuffer, VkFormat format,
    VkSampleCountFlagBits samples, VkExtent2D extent) {
  return TextureBuilder()
      .withAspect(hasStencil(format) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
                                       VK_IMAGE_ASPECT_DEPTH_BIT)
      .withExtent(extent.width, extent.height)
      .withFormat(format)
      .withNumSamples(samples)
      .withUsage(
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
      .withLayout(hasStencil(format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                                       VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
      .buildAttachment(logicalDevice, commandBuffer);
}

}  // namespace

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromSwapchain(
    VkCommandBuffer commandBuffer, const Renderpass& renderpass, VkExtent2D swapchainExtent,
    VkImageView swapchainImageView, std::vector<Texture>& attachments) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  std::span<const VkAttachmentDescription> attachmentDescriptions =
      renderpass.getAttachmentsLayout().getVkAttachmentDescriptions();

  std::vector<VkImageView> imageViews;
  imageViews.reserve(attachmentDescriptions.size());
  for (const VkAttachmentDescription& description : attachmentDescriptions) {
    switch (description.finalLayout) {
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        imageViews.push_back(swapchainImageView);
        break;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        {
          ASSIGN_OR_RETURN(Texture attachment,
                           createColorAttachment(logicalDevice, commandBuffer, description.format,
                                                 description.samples, swapchainExtent));
          imageViews.push_back(attachment.getVkImageView());
          attachments.push_back(std::move(attachment));
          break;
        }
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        {
          ASSIGN_OR_RETURN(Texture attachment,
                           createDepthAttachment(logicalDevice, commandBuffer, description.format,
                                                 description.samples, swapchainExtent));
          imageViews.push_back(attachment.getVkImageView());
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
  CHECK_VKCMD(vkCreateFramebuffer(
      renderpass.getLogicalDevice().getVkDevice(), &framebufferInfo, nullptr, &framebuffer));

  const VkViewport viewport = {
    .width = static_cast<float>(swapchainExtent.width),
    .height = static_cast<float>(swapchainExtent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f};
  const VkRect2D scissor = {.extent = swapchainExtent};
  return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor));
}

ErrorOr<std::unique_ptr<Framebuffer>> Framebuffer::createFromTextures(
    const Renderpass& renderpass, std::span<const Texture> textures) {
  std::vector<VkImageView> imageViews;
  imageViews.reserve(textures.size());
  std::optional<VkExtent2D> extent;
  for (const Texture& texture : textures) {
    imageViews.push_back(texture.getVkImageView());
    if (!extent.has_value()) {
      extent = texture.getVkExtent2D();
    } else if (VkExtent2D tmpExtent = texture.getVkExtent2D();
               extent->width != tmpExtent.width || extent->height != tmpExtent.height) {
      return Error(EngineError::SIZE_MISMATCH);
    }
  }

  if (!extent.has_value()) {
    return Error(EngineError::EMPTY_COLLECTION);
  }

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
  CHECK_VKCMD(vkCreateFramebuffer(
      renderpass.getLogicalDevice().getVkDevice(), &framebufferInfo, nullptr, &framebuffer));

  const VkViewport viewport = {
    .width = static_cast<float>(extent->width),
    .height = static_cast<float>(extent->height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f};

  const VkRect2D scissor = {.extent = *extent};
  return std::unique_ptr<Framebuffer>(new Framebuffer(framebuffer, renderpass, viewport, scissor));
}

Framebuffer::Framebuffer(VkFramebuffer framebuffer, const Renderpass& renderpass,
                         const VkViewport& viewport, const VkRect2D& scissor)
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
