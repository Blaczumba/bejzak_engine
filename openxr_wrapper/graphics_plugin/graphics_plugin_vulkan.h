#pragma once

#include <memory>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <span>
#include <unordered_map>
#include <vulkan/vulkan.h>  // Vulkan needs to be included before openxr_platform.h

#include "common/status/status.h"
#include "graphics_plugin.h"
#include "vulkan_wrapper/command_buffer/command_buffer.h"
#include "vulkan_wrapper/debug_messenger/debug_messenger.h"
#include "vulkan_wrapper/instance/instance.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/physical_device/physical_device.h"

namespace xrw {

class GraphicsPluginVulkan : public GraphicsPlugin {
public:
  GraphicsPluginVulkan(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

  ~GraphicsPluginVulkan() override = default;

  std::span<const char* const> getOpenXrInstanceExtensions() const override;

  const XrBaseInStructure* getGraphicsBinding() const override;

  XrSwapchainImageBaseHeader* allocateSwapchainImageStructs(
      uint32_t capacity, const XrSwapchainCreateInfo& swapchain_create_info) override;

  ErrorOr<int64_t> selectSwapchainFormat(std::span<const int64_t> runtimeFormats) const override;

  Status createSwapchainViews(
      XrSwapchain swapchain, std::span<const XrSwapchainImageBaseHeader> images, int64_t format,
      uint32_t width, uint32_t height) override;

  Status initialize(XrInstance xrInstance, XrSystemId systemId) override;

private:
  XrGraphicsBindingVulkanKHR _graphicsBinding;

  Instance _instance;
  PFN_vkDebugUtilsMessengerCallbackEXT _debugCallback;
#ifdef VALIDATION_LAYERS_ENABLED
  DebugMessenger _debugMessenger;
#endif
  std::unique_ptr<PhysicalDevice> _physicalDevice;
  LogicalDevice _logicalDevice;

  std::unordered_map<XrSwapchain, lib::Buffer<VkImageView>> _swapchainImageViews;

  std::unique_ptr<CommandPool> _singleTimeCommandPool;
};

}  // namespace xrw
