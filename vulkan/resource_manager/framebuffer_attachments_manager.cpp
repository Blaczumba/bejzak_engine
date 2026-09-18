#include "vulkan/resource_manager/framebuffer_attachments_manager.h"

#include <span>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/framebuffer/framebuffer.h"

Ref<Framebuffer> FramebufferManager::storeFramebuffer(
    Framebuffer&& framebuffer, const FramebufferMetadata& metadata,
    std::span<const Ref<Image>> attachments, VkImageView swapchainView) {
  _dependencies.emplace(
      framebuffer.getVkFramebuffer(),
      FramebufferDependencies{
        .imageRefs = lib::Buffer<Ref<Image>>(std::cbegin(attachments), std::cend(attachments)),
        .swapchainImageView = swapchainView});
  _allocationStrategy.transferResource(std::move(framebuffer), metadata);
}
