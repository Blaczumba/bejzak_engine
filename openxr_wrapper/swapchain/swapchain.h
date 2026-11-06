#pragma once

#include <openxr/openxr.h>
#include <vector>

#include "lib/buffer/buffer.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/session/session.h"

namespace xrw {

class Swapchain {
  Swapchain(XrSwapchain swapchain, uint32_t width, uint32_t height, std::vector<uint32_t>&& viewIndices) noexcept;

public:
  Swapchain() noexcept = default;

  Swapchain(Swapchain&& swapchain) noexcept;

  Swapchain& operator=(Swapchain&& swapchain) noexcept;

  ~Swapchain();

  XrSwapchain getSwapchain() const noexcept;

  XrExtent2Di getXrExtent2Di() const noexcept;

  uint32_t getArraySize() const noexcept;

  std::span<const uint32_t> getViewIndices() const noexcept;

private:
  XrSwapchain _swapchain = XR_NULL_HANDLE;

  uint32_t _width;
  uint32_t _height;
  std::vector<uint32_t> _viewIndices;

  friend class SwapchainBuilder;
};

class SwapchainBuilder {
public:
  SwapchainBuilder& withExtent(uint32_t width, uint32_t height);

  SwapchainBuilder& withMipCount(uint32_t mipCount);

  SwapchainBuilder& withFaceCount(uint32_t faceCount);

  SwapchainBuilder& withSampleCount(uint32_t sampleCount);

  SwapchainBuilder& withViewConfigType(XrViewConfigurationType viewConfigType);

  SwapchainBuilder& withUsage(XrSwapchainUsageFlags usage);

  std::vector<Swapchain> build(const Session& session, GraphicsPlugin& graphicsPlugin);

private:
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
