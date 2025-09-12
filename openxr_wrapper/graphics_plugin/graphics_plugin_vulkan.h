#pragma once

#include <vulkan/vulkan.h>  // Vulkan needs to be included before openxr_platform.h

#include <memory>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <span>
#include <unordered_map>
#include <vector>

#include "common/status/status.h"
#include "graphics_plugin.h"
#include "vulkan_wrapper/command_buffer/command_buffer.h"
#include "vulkan_wrapper/debug_messenger/debug_messenger.h"
#include "vulkan_wrapper/framebuffer/framebuffer.h"
#include "vulkan_wrapper/instance/instance.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/memory_objects/texture.h"
#include "vulkan_wrapper/physical_device/physical_device.h"

namespace xrw {

class GraphicsPluginVulkan : public GraphicsPlugin {
public:
  GraphicsPluginVulkan(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  ~GraphicsPluginVulkan() override;

  std::span<const char* const> getOpenXrInstanceExtensions() const override;

  const XrBaseInStructure* getGraphicsBinding() const override;

  ErrorOr<int64_t> selectSwapchainFormat(std::span<const int64_t> runtimeFormats) const override;

  Status createSwapchainContext(
      XrSwapchain swapchain, int64_t format, uint32_t width, uint32_t height) override;

  ErrorOr<XrSwapchainImageBaseHeader*> getSwapchainImages(XrSwapchain swapchain) override;

  Status initialize(XrInstance xrInstance, XrSystemId systemId) override;

  Status createResources() override;

  Status draw(XrSwapchain swapchain, uint32_t swapchain_image_index) override;

protected:
  XrGraphicsBindingVulkanKHR _graphicsBinding;

  Instance _instance;
  PFN_vkDebugUtilsMessengerCallbackEXT _debugCallback;
  DebugMessenger _debugMessenger;
  std::unique_ptr<PhysicalDevice> _physicalDevice;
  LogicalDevice _logicalDevice;

  static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
  static constexpr size_t MAX_THREADS_IN_POOL = 1;

  struct SwapchainContext {
    VkFormat format;
    uint32_t width;
    uint32_t height;
    lib::Buffer<XrSwapchainImageVulkanKHR> images;
    lib::Buffer<VkImageView> views;
    lib::Buffer<Framebuffer> framebuffers;
    std::vector<Texture> attachments;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> fences;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<std::shared_ptr<CommandPool>, MAX_THREADS_IN_POOL + 1>
        commandPools;
    std::array<PrimaryCommandBuffer, MAX_FRAMES_IN_FLIGHT> primaryCommandBuffer;
    std::array<std::array<SecondaryCommandBuffer, MAX_FRAMES_IN_FLIGHT>,
               MAX_THREADS_IN_POOL> commandBuffers;
  };

  std::unordered_map<XrSwapchain, SwapchainContext> _swapchainImageContexts;

  std::unique_ptr<CommandPool> _singleTimeCommandPool;
};

}  // namespace xrw
