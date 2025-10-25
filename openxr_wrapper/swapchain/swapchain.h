#pragma once

#include <memory>
#include <openxr/openxr.h>
#include <vector>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/session/session.h"
#include "openxr_wrapper/system/system.h"

namespace xrw {

class Swapchain {
public:
  Swapchain(XrSwapchain swapchain, XrViewConfigurationType configType, uint32_t width,
            uint32_t height, int64_t format, const Session& session);

  XrSwapchain getSwapchain() const;

  XrExtent2Di getXrExtent2Di() const;

private:
  XrSwapchain _swapchain;
  XrViewConfigurationType _configType;

  const Session& _session;

  int64_t _format;
  uint32_t _width;
  uint32_t _height;
};

class SwapchainBuilder {
public:
  SwapchainBuilder& withArraySize(uint32_t arraySize);

  SwapchainBuilder& withExtent(uint32_t width, uint32_t height);

  SwapchainBuilder& withMipCount(uint32_t mipCount);

  SwapchainBuilder& withFaceCount(uint32_t faceCount);

  SwapchainBuilder& withSampleCount(uint32_t sampleCount);

  SwapchainBuilder& withViewConfigType(XrViewConfigurationType viewConfigType);

  SwapchainBuilder& withUsage(XrSwapchainUsageFlags usage);

  ErrorOr<std::vector<Swapchain>> build(const Session& session, GraphicsPlugin& graphicsPlugin);

private:
  uint32_t _arraySize = 1;
  uint32_t _format = 0;
  uint32_t _width = 0;
  uint32_t _height = 0;
  uint32_t _mipCount = 1;
  uint32_t _faceCount = 1;
  uint32_t _sampleCount = 1;
  XrViewConfigurationType _viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  XrSwapchainUsageFlags _usage;
};

}  // namespace xrw
