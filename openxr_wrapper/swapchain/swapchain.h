#pragma once

#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "common/status/status.h"
#include "openxr_wrapper/session/session.h"
#include "openxr_wrapper/system/system.h"

#include <openxr/openxr.h>

#include <memory>

namespace xrw {

class Swapchain {
    Swapchain();

 public:


 private:
    XrSwapchain _swapchain;
    const Session& session;
};

class SwapchainBuilder {
 public:
    SwapchainBuilder& withArraySize(uint32_t arraySize);

    SwapchainBuilder& withExtent(int32_t widht, int32_t heith);

    SwapchainBuilder& withExtent(XrExtent2Di);

    SwapchainBuilder& withMipCount(uint32_t mipCount);

    SwapchainBuilder& withFaceCount(uint32_t faceCount);

    SwapchainBuilder& sampleCount(uint32_t sampleCount);

    SwapchainBuilder& withUsage(XrSwapchainUsageFlags usage);

    ErrorOr<Swapchain> build(const Session& session, const GraphicsPlugin& graphicsPlugin);

 private:
    uint32_t _arraySize = 1;
    uint32_t _format = 0;
    XrExtent2Di _extent = { 0, 0 };
    uint32_t _mipCount = 1;
    uint32_t _faceCount = 1;
    uint32_t _sampleCount = 1;
    XrSwapchainUsageFlags _usage;
};

} // xrw