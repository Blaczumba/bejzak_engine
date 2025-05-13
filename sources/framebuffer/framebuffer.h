#pragma once

#include "lib/status/status.h"
#include "lib/buffer/buffer.h"
#include "render_pass/render_pass.h"

#include <vulkan/vulkan.h>

#include <initializer_list>
#include <memory>
#include <optional>
#include <vector>

class CommandPool;
class Swapchain;

class Framebuffer {
	VkFramebuffer _framebuffer;

	const Renderpass& _renderpass;

	VkViewport _viewport;
	VkRect2D _scissor;
	lib::Buffer<std::shared_ptr<Texture>> _textureAttachments;

	Framebuffer(const VkFramebuffer framebuffer, const Renderpass& renderpass, const VkViewport& viewport, const VkRect2D& scissor, lib::Buffer<std::shared_ptr<Texture>>&& textures);

public:
	static lib::ErrorOr<std::unique_ptr<Framebuffer>> createFromSwapchain(const Renderpass& renderpass, const Swapchain& swapchain, const CommandPool& commandPool, uint8_t swapchainImageIndex);
	static lib::ErrorOr<std::unique_ptr<Framebuffer>> createFromTextures(const Renderpass& renderpass, lib::Buffer<std::shared_ptr<Texture>>&& textures);

	~Framebuffer();

	VkExtent2D getVkExtent() const;
	const VkViewport& getViewport() const;
	const VkRect2D& getScissor() const;
	const Renderpass& getRenderpass() const;
	VkFramebuffer getVkFramebuffer() const;
};
