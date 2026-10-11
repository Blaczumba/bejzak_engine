#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

class Framebuffer {
  Framebuffer(const Renderpass& renderpass, VkFramebuffer framebuffer) noexcept;

public:
  Framebuffer() noexcept = default;

  Framebuffer(Framebuffer&& framebuffer) noexcept;

  Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

  ~Framebuffer();

  static Framebuffer create(
      const Renderpass& renderpass, const VkFramebufferCreateInfo& createInfo);

  VkFramebuffer getVkFramebuffer() const noexcept;

  VkFramebuffer getUnderlyingResource() const noexcept;

  const Renderpass& getRenderpass() const;

private:
  void destroy();

  VkFramebuffer _framebuffer = VK_NULL_HANDLE;

  const Renderpass* _renderpass = nullptr;
};

struct FramebufferMetadata {
  VkFramebufferCreateFlags flags;
  VkRenderPass renderpass;
  lib::Buffer<VkImageView> attachments;
  VkExtent2D extent;
  uint32_t layers;
  // Other std::optional fields representing pNext metadata.
};

class FramebufferBuilder {
public:
  FramebufferBuilder&& addAttachment(VkImageView attachment) &&;

  FramebufferBuilder&& addAttachments(std::initializer_list<VkImageView> attachments) &&;

  FramebufferBuilder&& addAttachments(std::span<VkImageView> attachments) &&;

  FramebufferBuilder&& withAttachments(std::initializer_list<VkImageView> attachments) &&;

  FramebufferBuilder&& withAttachments(std::span<const VkImageView> attachments) &&;

  FramebufferBuilder&& withExtent(VkExtent2D extent) && noexcept;

  FramebufferBuilder&& withExtent(uint32_t width, uint32_t height) && noexcept;

  FramebufferBuilder&& withLayers(uint32_t layers) && noexcept;

  FramebufferBuilder&& withFlags(VkFramebufferCreateFlags flags) && noexcept;

  Framebuffer buildFramebuffer(const Renderpass& renderpass) &&;

  std::tuple<Framebuffer, FramebufferMetadata> buildFramebufferWithMetadata(
      const Renderpass& renderpass) &&;

private:
  FramebufferMetadata buildMetadataImpl() const noexcept;

  std::vector<VkImageView> _attachments;
  VkFramebufferCreateInfo _createInfo{
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .layers = 1};
};
