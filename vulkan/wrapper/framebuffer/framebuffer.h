#pragma once

#include <cstdint>
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

  VkFramebuffer getVkResource() const noexcept;

  const Renderpass& getRenderpass() const;

private:
  void destroy();

  VkFramebuffer _framebuffer = VK_NULL_HANDLE;

  const Renderpass* _renderpass = nullptr;
};

struct FramebufferMetadata {
  VkExtent2D extent;
  uint32_t layers;
  VkFramebufferCreateFlags flags;
  // Other std::optional fields representing pNext metadata.
};

class FramebufferBuilder {
public:
  FramebufferBuilder& addAttachment(VkImageView attachment);

  FramebufferBuilder& withAttachments(std::span<const VkImageView> attachments);

  FramebufferBuilder& withAttachments(std::initializer_list<VkImageView> attachments);

  FramebufferBuilder& withAttachments(std::vector<VkImageView>&& attachments) noexcept;

  FramebufferMetadata getMetadata() const noexcept;

  Framebuffer build(const Renderpass& renderpass, VkExtent2D extent, uint32_t layers,
                    VkFramebufferCreateFlags flags = 0);

private:
  std::vector<VkImageView> _attachments;
  VkExtent2D _extent;
  uint32_t _layers;
  VkFramebufferCreateFlags _flags;

  void* _pNext = nullptr;
};
