#include "swapchain.h"

#include "common/util/engine_exception.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

namespace {

struct SwapchainGroup {
  XrViewConfigurationView configurationView;
  std::vector<uint32_t> viewIndices;
};

bool areConfigurationViewsCompatible(
    const XrViewConfigurationView& a, const XrViewConfigurationView& b) {
  return a.recommendedImageRectWidth == b.recommendedImageRectWidth
         && a.recommendedImageRectHeight == b.recommendedImageRectHeight
         && a.recommendedSwapchainSampleCount == b.recommendedSwapchainSampleCount;
}

}  // namespace

Swapchain::Swapchain(XrSwapchain swapchain, uint32_t width, uint32_t height) noexcept
  : _swapchain(swapchain), _width(width), _height(height) {}

Swapchain Swapchain::create(XrSession session, const XrSwapchainCreateInfo& createInfo) {
  XrSwapchain swapchain;
  CHECK_XRCMD(xrCreateSwapchain(session, &createInfo, &swapchain), "Failed to create XrSwapchain.");
  return Swapchain(swapchain, createInfo.width, createInfo.height);
}

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

SwapchainBuilder&& SwapchainBuilder::withUsage(XrSwapchainUsageFlags usage) && noexcept  {
  _createInfo.usageFlags = usage;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withFormat(int64_t format) && noexcept {
  _createInfo.format = format;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withSampleCount(uint32_t sampleCount) && noexcept {
  _createInfo.sampleCount = sampleCount;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withExtent(uint32_t width, uint32_t height) && noexcept {
  _createInfo.width = width;
  _createInfo.height = height;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withFaceCount(uint32_t faceCount) && noexcept {
  _createInfo.faceCount = faceCount;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withArraySize(uint32_t arraySize) && noexcept {
  _createInfo.arraySize = arraySize;
  return std::move(*this);
}

SwapchainBuilder&& SwapchainBuilder::withMipCount(uint32_t mipCount) && noexcept {
  _createInfo.mipCount = mipCount;
  return std::move(*this);
}

Swapchain SwapchainBuilder::build(const Session& session) && {
  return Swapchain::create(session.getXrSession(), _createInfo);
}

}  // namespace xrw
