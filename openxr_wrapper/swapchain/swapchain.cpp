#include "swapchain.h"

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

SwapchainBuilder& SwapchainBuilder::withArraySize(uint32_t arraySize) {
  _arraySize = arraySize;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withExtent(int32_t width, int32_t height) {
  _extent = {width, height};
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withExtent(XrExtent2Di extent) {
  _extent = extent;
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

SwapchainBuilder& SwapchainBuilder::sampleCount(uint32_t sampleCount) {
  _sampleCount = sampleCount;
  return *this;
}

SwapchainBuilder& SwapchainBuilder::withUsage(XrSwapchainUsageFlags usage) {
  _usage = usage;
  return *this;
}

ErrorOr<Swapchain> build(const Session& session, const GraphicsPlugin& graphicsPlugin) {
  uint32_t formatCount = 0;
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session.getXrSession(), 0, &formatCount, nullptr));
  lib::Buffer<int64_t> swapchainFormats(formatCount);
  CHECK_XRCMD(xrEnumerateSwapchainFormats(
      session.getXrSession(), static_cast<uint32_t>(swapchainFormats.size()), &formatCount,
      swapchainFormats.data()));
  ASSIGN_OR_RETURN(const int64_t format, graphicsPlugin.selectSwapchainFormat(swapchainFormats));

  const XrInstance instance = session.getSystem().getInstance().getXrInstance();
  const XrSystemId systemId = session.getSystem().getXrSystemId();
  static constexpr XrViewConfigurationType viewConfigType =
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  uint32_t viewCount = 0;
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(
      instance, systemId, viewConfigType, 0, &viewCount, nullptr));
  lib::Buffer<XrViewConfigurationView> configurationViews(viewCount);
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(
      instance, systemId, viewConfigType, viewCount, &viewCount, configurationViews.data()));

  // lib::Buffer<XrView>
  return Error(EngineError::NOT_FOUND);
}

}  // namespace xrw
