#pragma once

#include <openxr/openxr.h>
#include <vector>

#include "openxr_wrapper/session/session.h"

namespace xrw {

class Swapchain {
  Swapchain(XrSwapchain swapchain, uint32_t width, uint32_t height) noexcept;

public:
  Swapchain() noexcept = default;

  static Swapchain create(XrSession session, const XrSwapchainCreateInfo& createInfo);

  Swapchain(Swapchain&& swapchain) noexcept;

  Swapchain& operator=(Swapchain&& swapchain) noexcept;

  ~Swapchain();

  XrSwapchain getSwapchain() const noexcept;

  XrExtent2Di getXrExtent2Di() const noexcept;

  uint32_t getArraySize() const noexcept;

private:
  XrSwapchain _swapchain = XR_NULL_HANDLE;

  uint32_t _width;
  uint32_t _height;
};

class SwapchainBuilder {
public:
  SwapchainBuilder&& withUsage(XrSwapchainUsageFlags usage) && noexcept;

  SwapchainBuilder&& withFormat(int64_t format) && noexcept;

  SwapchainBuilder&& withSampleCount(uint32_t sampleCount) && noexcept;

  SwapchainBuilder&& withExtent(uint32_t width, uint32_t height) && noexcept;

  SwapchainBuilder&& withFaceCount(uint32_t faceCount) && noexcept;

  SwapchainBuilder&& withArraySize(uint32_t arraySize) && noexcept;

  SwapchainBuilder&& withMipCount(uint32_t mipCount) && noexcept;

  Swapchain build(const Session& session) &&;

private:
  XrSwapchainCreateInfo _createInfo{
    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
    .sampleCount = 1,
    .faceCount = 1,
    .arraySize = 1,
    .mipCount = 1};
};

}  // namespace xrw
