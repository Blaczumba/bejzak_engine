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

Swapchain::Swapchain(XrSwapchain swapchain, uint32_t width, uint32_t height,
                     lib::Buffer<uint32_t>&& viewIndices) noexcept
  : _swapchain(swapchain), _width(width), _height(height), _viewIndices(std::move(viewIndices)) {}

Swapchain Swapchain::create(XrSession session, const XrSwapchainCreateInfo& createInfo,
                            std::span<const uint32_t> viewIndices) {
  XrSwapchain swapchain;
  CHECK_XRCMD(xrCreateSwapchain(session, &createInfo, &swapchain),
              "Failed to create XrSwapchain.");
  return Swapchain(swapchain, createInfo.width, createInfo.height, viewIndices);
}

Swapchain::Swapchain(Swapchain&& swapchain) noexcept
  : _swapchain(std::exchange(swapchain._swapchain, XR_NULL_HANDLE)), _width(swapchain._width),
    _height(swapchain._height), _viewIndices(std::move(swapchain._viewIndices)) {}

Swapchain& Swapchain::operator=(Swapchain&& swapchain) noexcept {
  if (this == &swapchain) {
    return *this;
  }

  _swapchain = std::exchange(swapchain._swapchain, XR_NULL_HANDLE);
  _width = swapchain._width;
  _height = swapchain._height;
  _viewIndices = std::move(swapchain._viewIndices);
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

uint32_t Swapchain::getArraySize() const noexcept {
  return _viewIndices.size();
}

std::span<const uint32_t> Swapchain::getViewIndices() const noexcept {
  return _viewIndices;
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

  std::vector<SwapchainGroup> swapchainGroup;
  for (uint32_t i = 0; i < configurationViews.size(); i++) {
    auto it = std::find_if(
        swapchainGroup.begin(), swapchainGroup.end(), [&](const SwapchainGroup& swapchainGroup) {
          return areConfigurationViewsCompatible(
              swapchainGroup.configurationView, configurationViews[i]);
        });
    if (it == swapchainGroup.cend()) {
      swapchainGroup.emplace_back(configurationViews[i], std::vector<uint32_t>{i});
    } else {
      it->viewIndices.push_back(i);
    }
  }

  std::vector<Swapchain> swapchains;
  swapchains.reserve(swapchainGroup.size());
  for (SwapchainGroup& swapchainGroup : swapchainGroup) {
    const XrViewConfigurationView& configurationView = swapchainGroup.configurationView;
    std::span<const uint32_t> viewIndices = swapchainGroup.viewIndices;
    const XrSwapchainCreateInfo swapchainCreateInfo = {
      .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
      .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
      .format = *format,
      .sampleCount = configurationView.recommendedSwapchainSampleCount,
      .width = configurationView.recommendedImageRectWidth,
      .height = configurationView.recommendedImageRectHeight,
      .faceCount = 1,
      .arraySize = static_cast<uint32_t>(viewIndices.size()),
      .mipCount = 1};

    Swapchain& swapchain = swapchains.emplace_back(
        Swapchain::create(session.getXrSession(), swapchainCreateInfo, viewIndices));

    graphicsPlugin.createSwapchainContext(
        swapchain.getSwapchain(), *format, configurationView.recommendedImageRectWidth,
        configurationView.recommendedImageRectHeight, viewIndices.size());
  }
  return swapchains;
}

}  // namespace xrw
