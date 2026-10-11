#include "framebuffer.h"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

Framebuffer::Framebuffer(const Renderpass& renderpass, VkFramebuffer framebuffer) noexcept
  : _framebuffer(framebuffer), _renderpass(&renderpass) {}

Framebuffer::Framebuffer(Framebuffer&& framebuffer) noexcept
  : _framebuffer(std::exchange(framebuffer._framebuffer, VK_NULL_HANDLE)),
    _renderpass(framebuffer._renderpass) {}

void Framebuffer::destroy() {
  if (_framebuffer != VK_NULL_HANDLE) {
    _renderpass->getLogicalDevice().destroyResource(
        [framebuffer = _framebuffer](DestroyerContext context) {
          vkDestroyFramebuffer(context.device, framebuffer, context.allocationCallbacks);
        });
    _framebuffer = VK_NULL_HANDLE;
  }
}

Framebuffer& Framebuffer::operator=(Framebuffer&& framebuffer) noexcept {
  if (&framebuffer == this) {
    return *this;
  }

  destroy();

  _framebuffer = std::exchange(framebuffer._framebuffer, VK_NULL_HANDLE);
  _renderpass = std::exchange(framebuffer._renderpass, nullptr);
  return *this;
}

Framebuffer::~Framebuffer() {
  destroy();
}

Framebuffer Framebuffer::create(
    const Renderpass& renderpass, const VkFramebufferCreateInfo& createInfo) {
  VkFramebuffer framebuffer;
  CHECK_VKCMD(vkCreateFramebuffer(
                  renderpass.getLogicalDevice().getVkDevice(), &createInfo, nullptr, &framebuffer),
              "Failed to create VkFramebuffer.");

  return Framebuffer(renderpass, framebuffer);
}

VkFramebuffer Framebuffer::getVkFramebuffer() const noexcept {
  return _framebuffer;
}

VkFramebuffer Framebuffer::getUnderlyingResource() const noexcept {
  return _framebuffer;
}

const Renderpass& Framebuffer::getRenderpass() const {
  return *_renderpass;
}

FramebufferBuilder&& FramebufferBuilder::addAttachment(VkImageView attachment) && {
  _attachments.push_back(attachment);
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::addAttachments(
    std::initializer_list<VkImageView> attachments) && {
  _attachments.append_range(attachments);
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::addAttachments(std::span<VkImageView> attachments) && {
  _attachments.append_range(attachments);
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withAttachments(
    std::initializer_list<VkImageView> attachments) && {
  _attachments.assign_range(attachments);
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withAttachments(
    std::span<const VkImageView> attachments) && {
  _attachments.assign_range(attachments);
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withExtent(VkExtent2D extent) && noexcept {
  _createInfo.width = extent.width;
  _createInfo.height = extent.height;
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withExtent(uint32_t width, uint32_t height) && noexcept {
  _createInfo.width = width;
  _createInfo.height = height;
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withLayers(uint32_t layers) && noexcept {
  _createInfo.layers = layers;
  return std::move(*this);
}

FramebufferBuilder&& FramebufferBuilder::withFlags(VkFramebufferCreateFlags flags) && noexcept {
  _createInfo.flags = flags;
  return std::move(*this);
}

FramebufferMetadata FramebufferBuilder::buildMetadataImpl() const noexcept {
  return FramebufferMetadata{
    .flags = _createInfo.flags,
    .renderpass = _createInfo.renderPass,
    .attachments = std::span(_attachments),
    .extent = {_createInfo.width, _createInfo.height},
    .layers = _createInfo.layers
  };
}

Framebuffer FramebufferBuilder::buildFramebuffer(const Renderpass& renderpass) && {
  return Framebuffer::create(renderpass, _createInfo);
}

std::tuple<Framebuffer, FramebufferMetadata> FramebufferBuilder::buildFramebufferWithMetadata(
    const Renderpass& renderpass) && {
  _createInfo.renderPass = renderpass.getVkRenderPass();
  _createInfo.attachmentCount = static_cast<uint32_t>(_attachments.size());
  _createInfo.pAttachments = _attachments.data();
  return {Framebuffer::create(renderpass, _createInfo), buildMetadataImpl()};
}
