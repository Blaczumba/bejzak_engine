#pragma once

#if ANDROID
  #include <jni.h>
#endif
#include <memory>
#include <openxr/openxr.h>
#include <vulkan/vulkan.h>  // Vulkan needs to be included before openxr_platform.h
#include <openxr/openxr_platform.h>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "graphics_plugin.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"

namespace xrw {

class GraphicsPluginVulkan final : public GraphicsPlugin {
public:
  GraphicsPluginVulkan() noexcept = default;

  ~GraphicsPluginVulkan() override;

  std::span<const char* const> getOpenXrInstanceExtensions() const override;

  const XrBaseInStructure* getGraphicsBinding() const override;

  std::optional<int64_t> selectSwapchainFormat(
      std::span<const int64_t> runtimeFormats) const override;

  void createSwapchainContext(XrSwapchain swapchain, int64_t format, uint32_t width,
                              uint32_t height, uint32_t layerCount) override;

  common::PresentResources getSwapchainContext(XrSwapchain swapchain) override;

  void initialize(XrInstance xrInstance, XrSystemId systemId) override;

  void createResources() override;

  std::unique_ptr<common::GraphicsContext> createGraphicsContext(
      XrInstance xrInstance, XrSystemId systemId, std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer, const FileLoader& fileLoader) override;

private:
  const LogicalDevice* _logicalDevice;

  XrGraphicsBindingVulkanKHR _graphicsBinding;

  std::unordered_map<XrSwapchain, common::PresentResources> _presentResources;
  std::unordered_map<XrSwapchain, lib::Buffer<VkImageView>> _swapchainViews;
};

}  // namespace xrw
