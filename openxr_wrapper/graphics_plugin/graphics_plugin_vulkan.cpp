#include "graphics_plugin_vulkan.h"

#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

#include <array>
#include <span>

namespace xrw {

std::span<const char *const> GraphicsPluginVulkan::getOpenXrInstanceExtensions() const {
    static constexpr std::array instanceExtensions = {XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};
    return instanceExtensions;
}

const XrBaseInStructure* GraphicsPluginVulkan::getGraphicsBinding() const {
    return nullptr;
}

XrSwapchainImageBaseHeader* GraphicsPluginVulkan::allocateSwapchainImageStructs(uint32_t capacity,
                                                          const XrSwapchainCreateInfo &swapchain_create_info) {
  return nullptr;
}

} //xrw