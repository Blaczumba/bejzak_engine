#include "swapchain.h"

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Swapchain::Swapchain(XrSwapchain swapchain, XrViewConfigurationType configType, uint32_t width,
                     uint32_t height, int64_t format, const Session& session)
  : _swapchain(swapchain), _configType(configType), _width(width), _height(height), _format(format),
    _session(session) {}

XrSwapchain Swapchain::getSwapchain() const {
  return _swapchain;
}

XrExtent2Di Swapchain::getXrExtent2Di() const {
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

ErrorOr<std::vector<Swapchain>> SwapchainBuilder::build(
    const Session& session, GraphicsPlugin& graphicsPlugin) {
  uint32_t formatCount = 0;
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session.getXrSession(), 0, &formatCount, nullptr));
  lib::Buffer<int64_t> swapchainFormats(formatCount);
  CHECK_XRCMD(xrEnumerateSwapchainFormats(
      session.getXrSession(), static_cast<uint32_t>(swapchainFormats.size()), &formatCount,
      swapchainFormats.data()));
  ASSIGN_OR_RETURN(const int64_t format, graphicsPlugin.selectSwapchainFormat(swapchainFormats));

  const XrInstance instance = session.getSystem().getInstance().getXrInstance();
  const XrSystemId systemId = session.getSystem().getXrSystemId();
  uint32_t viewCount = 0;
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(
      instance, systemId, _viewConfigType, 0, &viewCount, nullptr));
  lib::Buffer<XrViewConfigurationView> configurationViews(
      viewCount, XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW});
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(
      instance, systemId, _viewConfigType, viewCount, &viewCount, configurationViews.data()));

  std::vector<Swapchain> swapchains;
  for (const XrViewConfigurationView& configView : configurationViews) {
    const XrSwapchainCreateInfo swapchainCreateInfo = {
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .format = format,
      .sampleCount = configView.recommendedSwapchainSampleCount,
      .width = configView.recommendedImageRectWidth,
      .height = configView.recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = 1,
      .mipCount = 1};

    XrSwapchain swapchain;
    CHECK_XRCMD(xrCreateSwapchain(session.getXrSession(), &swapchainCreateInfo, &swapchain));

    RETURN_IF_ERROR(graphicsPlugin.createSwapchainContext(
        swapchain, format, configView.recommendedImageRectWidth,
        configView.recommendedImageRectHeight));

    swapchains.emplace_back(swapchain, _viewConfigType, configView.recommendedImageRectWidth,
                            configView.recommendedImageRectHeight, format, session);
  }
  return swapchains;
}

}  // namespace xrw
