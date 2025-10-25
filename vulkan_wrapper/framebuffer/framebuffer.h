#pragma once

#include <initializer_list>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/render_pass/render_pass.h"

class Framebuffer {
  VkFramebuffer _framebuffer = VK_NULL_HANDLE;

  const Renderpass* _renderpass = nullptr;

  VkViewport _viewport;
  VkRect2D _scissor;

  Framebuffer(VkFramebuffer framebuffer, const Renderpass& renderpass, const VkViewport& viewport,
              const VkRect2D& scissor);

public:
  Framebuffer() = default;

  static ErrorOr<Framebuffer> createFromSwapchain(
      VkCommandBuffer commandBuffer, const Renderpass& renderpass, VkExtent2D swapchainExtent,
      VkImageView swapchainImageView, std::vector<Texture>& attachments);

  static ErrorOr<Framebuffer> createFromTextures(
      const Renderpass& renderpass, std::span<const Texture> textures);

  Framebuffer(Framebuffer&& framebuffer) noexcept;

  Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

  ~Framebuffer();

  VkExtent2D getVkExtent() const;

  const VkViewport& getViewport() const;

  const VkRect2D& getScissor() const;

  const Renderpass& getRenderpass() const;

  VkFramebuffer getVkFramebuffer() const;
};
