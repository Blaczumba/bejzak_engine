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
#include <initializer_list>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "graphics_plugin.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/physical_device/optional_extended_features.h"
#include "vulkan/wrapper/physical_device/physical_device.h"

namespace xrw {

class GraphicsPluginVulkan final : public GraphicsPlugin {
  GraphicsPluginVulkan(
      std::shared_ptr<Instance> instance, std::shared_ptr<DebugMessenger> debugMessenger,
      std::shared_ptr<PhysicalDevice> physicalDevice,
      PhysicalDeviceOptionalExtendedFeatures extendedFeatures,
      std::shared_ptr<LogicalDevice> logicalDevice) noexcept;

public:
  static std::unique_ptr<GraphicsPlugin> create(XrInstance xrInstance, XrSystemId xrSystemId);

  ~GraphicsPluginVulkan() override;

  const XrBaseInStructure* getGraphicsBinding() const override;

  std::optional<int64_t> selectFirstSupportedSwapchainFormat(XrSession session,
                                                             std::initializer_list<common::Format> formats) const override;

  std::optional<int64_t> isSupportedSwapchainFormat(XrSession session, common::Format format) const override;

  void createSwapchainContext(XrSwapchain swapchain, int64_t format, uint32_t width,
                              uint32_t height, uint32_t layerCount) override;

  common::PresentResources getSwapchainContext(XrSwapchain swapchain) override;

  void initialize(XrInstance xrInstance, XrSystemId systemId) override;

  void createResources() override;

  std::unique_ptr<common::GraphicsContext> createGraphicsContext(
      std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
      const FileLoader& fileLoader) override;

private:
  std::shared_ptr<Instance> _instance;
  std::shared_ptr<DebugMessenger> _debugMessenger;
  std::shared_ptr<PhysicalDevice> _physicalDevice;
  PhysicalDeviceOptionalExtendedFeatures _extendedFeatures;
  std::shared_ptr<LogicalDevice> _logicalDevice;

  XrGraphicsBindingVulkanKHR _graphicsBinding;

  std::unordered_map<XrSwapchain, common::PresentResources> _presentResources;
  std::unordered_map<XrSwapchain, lib::Buffer<VkImageView>> _swapchainViews;
};

}  // namespace xrw
