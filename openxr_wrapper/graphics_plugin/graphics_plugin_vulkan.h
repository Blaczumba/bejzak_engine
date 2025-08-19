#pragma once

#include <openxr/openxr.h>
#include <memory>
#include <span>

#include "common/status/status.h"
#include "graphics_plugin.h"

#include "vulkan_wrapper/instance/instance.h"
#include "vulkan_wrapper/debug_messenger/debug_messenger.h"
#include "vulkan_wrapper/physical_device/physical_device.h"
//#include "vulkan_wrapper/logical_device/logical_device.h"

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

  Status initialize(XrInstance xrInstance, XrSystemId systemId) override;

 private:
  Instance _instance;
  PFN_vkDebugUtilsMessengerCallbackEXT _debugCallback;
#ifdef VALIDATION_LAYERS_ENABLED
  DebugMessenger _debugMessenger;
# endif
  std::unique_ptr<PhysicalDevice> _physicalDevice;
  // LogicalDevice _logicalDevice;
};

}  // namespace xrw
