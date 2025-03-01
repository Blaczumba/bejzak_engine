#pragma once

#include "memory_objects/texture/texture_factory.h"
#include "render_pass/render_pass.h"

#include <vulkan/vulkan.h>

#include <initializer_list>
#include <memory>
#include <vector>

class CommandPool;
class Swapchain;

class Framebuffer {
	VkFramebuffer _framebuffer;
	std::vector<std::shared_ptr<Texture>> _textureAttachments;
	const std::optional<uint8_t> _swapchainIndex;
	VkViewport _viewport;
	VkRect2D _scissor;

	const Renderpass& _renderpass;

	Framebuffer(const Renderpass& renderpass, const Swapchain& swapchain, uint8_t swapchainImageIndex, const CommandPool& commandPool);
	Framebuffer(const Renderpass& renderpass, std::vector<std::shared_ptr<Texture>>&& textures);

public:
	static std::unique_ptr<Framebuffer> createFromSwapchain(const Renderpass& renderpass, const Swapchain& swapchain, uint8_t swapchainImageIndex, const CommandPool& commandPool) {
		return std::unique_ptr<Framebuffer>(new Framebuffer(renderpass, swapchain, swapchainImageIndex, commandPool));
	}
	
	// TODO change to unique_ptr
	template<typename... Textures>
	static std::unique_ptr<Framebuffer> createFromTextures(const Renderpass& renderpass, std::shared_ptr<Textures>&&... textures) {
		std::vector<std::shared_ptr<Texture>> tex;
		tex.reserve(sizeof...(textures));
		(tex.push_back(textures), ...);
		return std::unique_ptr<Framebuffer>(new Framebuffer(renderpass, std::move(tex)));
	}

	~Framebuffer();

	VkExtent2D getVkExtent() const;
	const VkViewport& getViewport() const;
	const VkRect2D& getScissor() const;
	const Renderpass& getRenderpass() const;
	VkFramebuffer getVkFramebuffer() const;
};
