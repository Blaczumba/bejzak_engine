#pragma once

#include "vulkan_wrapper/lib/buffer/buffer.h"
#include "vulkan_wrapper/render_pass/render_pass.h"
#include "vulkan_wrapper/status/status.h"

#include <vulkan/vulkan.h>

#include <initializer_list>
#include <vector>

class CommandPool;
class Swapchain;

class Framebuffer {
	VkFramebuffer _framebuffer;

	const Renderpass& _renderpass;

	VkViewport _viewport;
	VkRect2D _scissor;

	Framebuffer(VkFramebuffer framebuffer, const Renderpass& renderpass, const VkViewport& viewport, const VkRect2D& scissor);

public:
	static ErrorOr<std::unique_ptr<Framebuffer>> createFromSwapchain(VkCommandBuffer commandBuffer, const Renderpass& renderpass, VkExtent2D swapchainExtent, VkImageView swapchainImageView, std::vector<Texture>& attachments);
	
	static ErrorOr<std::unique_ptr<Framebuffer>> createFromTextures(const Renderpass& renderpass, std::span<const Texture> textures);

	~Framebuffer();

	VkExtent2D getVkExtent() const;
	
	const VkViewport& getViewport() const;
	
	const VkRect2D& getScissor() const;
	
	const Renderpass& getRenderpass() const;
	
	VkFramebuffer getVkFramebuffer() const;
};
