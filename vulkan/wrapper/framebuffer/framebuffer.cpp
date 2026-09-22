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

VkFramebuffer Framebuffer::getVkResource() const noexcept {
  return _framebuffer;
}

const Renderpass& Framebuffer::getRenderpass() const {
  return *_renderpass;
}

FramebufferBuilder& FramebufferBuilder::addAttachment(VkImageView attachment) {
  _attachments.push_back(attachment);
  return *this;
}

FramebufferBuilder& FramebufferBuilder::withAttachments(std::span<const VkImageView> attachments) {
  _attachments.assign_range(attachments);
  return *this;
}

FramebufferBuilder& FramebufferBuilder::withAttachments(
    std::initializer_list<VkImageView> attachments) {
  _attachments.assign_range(attachments);
  return *this;
}

FramebufferBuilder& FramebufferBuilder::withAttachments(
    std::vector<VkImageView>&& attachments) noexcept {
  _attachments = std::move(attachments);
  return *this;
}

FramebufferMetadata FramebufferBuilder::getMetadata() const noexcept {
  return FramebufferMetadata{.extent = _extent, .layers = _layers, .flags = _flags};
}

Framebuffer FramebufferBuilder::build(const Renderpass& renderpass, VkExtent2D extent,
                                      uint32_t layers, VkFramebufferCreateFlags flags) {
  const VkFramebufferCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .flags = _flags = flags,
    .renderPass = renderpass.getVkRenderPass(),
    .attachmentCount = static_cast<uint32_t>(_attachments.size()),
    .pAttachments = _attachments.data(),
    .width = _extent.width = extent.width,
    .height = _extent.height = extent.height,
    .layers = _layers = layers};
  return Framebuffer::create(renderpass, createInfo);
}
