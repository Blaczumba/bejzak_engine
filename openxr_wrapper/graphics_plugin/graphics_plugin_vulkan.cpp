#include "graphics_plugin_vulkan.h"

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include <algorithm>
#include <array>
#include <openxr/openxr_platform.h>
#include <span>

namespace xrw {

std::span<const char* const> GraphicsPluginVulkan::getOpenXrInstanceExtensions() const {
  static constexpr std::array instanceExtensions = {XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};
  return instanceExtensions;
}

const XrBaseInStructure* GraphicsPluginVulkan::getGraphicsBinding() const {
  return nullptr;
}

XrSwapchainImageBaseHeader* GraphicsPluginVulkan::allocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo& swapchain_create_info) {
  return nullptr;
}

ErrorOr<int64_t> GraphicsPluginVulkan::selectSwapchainFormat(
    std::span<const int64_t> runtimeFormats) const {
  static constexpr VkFormat preferredFromats[] = {
    VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM};

  if (auto it = std::find_first_of(std::cbegin(preferredFromats), std::cend(preferredFromats),
                                   std::cbegin(runtimeFormats), std::cend(runtimeFormats));
      it != std::cend(preferredFromats)) {
    return *it;
  }
  return Error(EngineError::NOT_FOUND);
}

}  // namespace xrw
