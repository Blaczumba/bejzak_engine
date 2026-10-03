#include "swapchain.h"

#include "common/util/engine_exception.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Swapchain::Swapchain(XrSwapchain swapchain, uint32_t width, uint32_t height) noexcept
  : _swapchain(swapchain), _width(width), _height(height) {}

Swapchain::Swapchain(Swapchain&& swapchain) noexcept
  : _swapchain(std::exchange(swapchain._swapchain, XR_NULL_HANDLE)), _width(swapchain._width),
    _height(swapchain._height) {}

Swapchain& Swapchain::operator=(Swapchain&& swapchain) noexcept {
  if (this == &swapchain) {
    return *this;
  }

  _swapchain = std::exchange(swapchain._swapchain, XR_NULL_HANDLE);
  _width = swapchain._width;
  _height = swapchain._height;
  return *this;
}

Swapchain::~Swapchain() {
  if (_swapchain != XR_NULL_HANDLE) {
    xrDestroySwapchain(_swapchain);
  }
}

XrSwapchain Swapchain::getSwapchain() const noexcept {
  return _swapchain;
}

XrExtent2Di Swapchain::getXrExtent2Di() const noexcept {
  return {static_cast<int32_t>(_width), static_cast<int32_t>(_height)};
}

SwapchainBuilder& SwapchainBuilder::withArraySize(uint32_t arraySize) {
  _arraySize = arraySize;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withExtent(uint32_t width, uint32_t height) {
  _width = width;
  _height = height;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withMipCount(uint32_t mipCount) {
  _mipCount = mipCount;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withFaceCount(uint32_t faceCount) {
  _faceCount = faceCount;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withSampleCount(uint32_t sampleCount) {
  _sampleCount = sampleCount;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withViewConfigType(XrViewConfigurationType viewConfigType) {
  _viewConfigType = viewConfigType;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withUsage(XrSwapchainUsageFlags usage) {
  _usage = usage;
  return *this;
}

std::vector<Swapchain> SwapchainBuilder::build(
    const Session& session, GraphicsPlugin& graphicsPlugin) {
  uint32_t formatCount = 0;
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session.getXrSession(), 0, &formatCount, nullptr),
              "Failed to xrEnumerateSwapchainFormats.");
  lib::Buffer<int64_t> swapchainFormats(formatCount);
  CHECK_XRCMD(xrEnumerateSwapchainFormats(
                  session.getXrSession(), static_cast<uint32_t>(swapchainFormats.size()),
                  &formatCount, swapchainFormats.data()),
              "Failed to xrEnumerateSwapchainFormats.");
  const std::optional<int64_t> format = *graphicsPlugin.selectSwapchainFormat(swapchainFormats);
  if (!format.has_value()) {
    throw EngineException("Could not find the format for swapchain images.");
  }

  const XrInstance instance = session.getSystem().getInstance().getXrInstance();
  const XrSystemId systemId = session.getSystem().getXrSystemId();
  uint32_t viewCount = 0;
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(
                  instance, systemId, _viewConfigType, 0, &viewCount, nullptr),
              "Failed to xrEnumerateViewConfigurationViews.");
  lib::Buffer<XrViewConfigurationView> configurationViews(
      viewCount, XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW});
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, _viewConfigType, viewCount,
                                                &viewCount, configurationViews.data()),
              "Failed to xrEnumerateViewConfigurationViews.");

  std::vector<XrViewConfigurationView> processedConfigViews;
  if (_arraySize == 2 && configurationViews.size() == 2) {
    // Multiview feature. Render using 1 swapchain instead of 2.
    // TODO: Check if configViews have the same properties.
    processedConfigViews.push_back(configurationViews[0]);
  } else {
    processedConfigViews = std::vector<XrViewConfigurationView>(
        configurationViews.cbegin(), configurationViews.cend());
  }

  std::vector<Swapchain> swapchains;
  for (const XrViewConfigurationView& configView : processedConfigViews) {
    const XrSwapchainCreateInfo swapchainCreateInfo = {
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .format = *format,
      .sampleCount = configView.recommendedSwapchainSampleCount,
      .width = configView.recommendedImageRectWidth,
      .height = configView.recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = _arraySize,
      .mipCount = 1};

    XrSwapchain swapchain;
    CHECK_XRCMD(xrCreateSwapchain(session.getXrSession(), &swapchainCreateInfo, &swapchain),
                "Failed to create XrSwapchain.");

    graphicsPlugin.createSwapchainContext(swapchain, *format, configView.recommendedImageRectWidth,
                                          configView.recommendedImageRectHeight, _arraySize);

    swapchains.push_back(Swapchain(
        swapchain, configView.recommendedImageRectWidth, configView.recommendedImageRectHeight));
  }
  return swapchains;
}

}  // namespace xrw
