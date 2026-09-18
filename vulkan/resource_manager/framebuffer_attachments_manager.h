#pragma once

#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/util/resource_handles.h"
#include "lib/buffer/buffer.h"
#include "vulkan/resource_manager/ref.h"
#include "vulkan/resource_manager/resource_manager_allocation_strategy.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/memory_objects/image.h"

class FramebufferManager {
public:
  FramebufferManager() noexcept = default;

  ~FramebufferManager() = default;

  Ref<Framebuffer> storeFramebuffer(
      Framebuffer&& framebuffer, const FramebufferMetadata& metadata,
      std::span<const Ref<Image>> attachments, VkImageView swapchainView = VK_NULL_HANDLE);

private:
  struct FramebufferDependencies {
    lib::Buffer<Ref<Image>> imageRefs;
    VkImageView swapchainImageView;
  };

  AllocationStrategy<Framebuffer, AllocationPolicy::POOL_BASED> _allocationStrategy;
  std::unordered_map<VkFramebuffer, FramebufferDependencies> _dependencies;
};
