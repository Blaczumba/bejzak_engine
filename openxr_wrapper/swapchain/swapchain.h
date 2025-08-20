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
            uint32_t height, int64_t format, lib::Buffer<XrSwapchainImageBaseHeader>&& images,
            const Session& session);

private:
  XrSwapchain _swapchain;
  XrViewConfigurationType _configType;
  lib::Buffer<XrSwapchainImageBaseHeader> _images;

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

  SwapchainBuilder& sampleCount(uint32_t sampleCount);

  SwapchainBuilder& withUsage(XrSwapchainUsageFlags usage);

  ErrorOr<std::vector<Swapchain>> buildStereo(
      const Session& session, const GraphicsPlugin& graphicsPlugin);

private:
  uint32_t _arraySize = 1;
  uint32_t _format = 0;
  uint32_t _width = 0;
  uint32_t _height = 0;
  uint32_t _mipCount = 1;
  uint32_t _faceCount = 1;
  uint32_t _sampleCount = 1;
  XrSwapchainUsageFlags _usage;
};

}  // namespace xrw
