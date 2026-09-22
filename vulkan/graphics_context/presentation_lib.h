#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>

#include "common/abstractions/contexts.h"
#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/builders/submit_info_builder.h"
#include "vulkan/wrapper/swapchain/swapchain.h"

namespace vlkn {

constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

class PresentationContext {
  PresentationContext(Swapchain&& swapchain) noexcept;

public:
  static std::unique_ptr<PresentationContext> create(Swapchain&& swapchain);

  ~PresentationContext();

  PresentationContext(const PresentationContext&) = delete;

  PresentationContext(PresentationContext&&) = delete;

  PresentationContext& operator=(const PresentationContext&) = delete;

  PresentationContext& operator=(PresentationContext&&) = delete;

  common::PresentResources getPresentResources() const;

  void synchronizeSubmit(SubmitInfoBuilder* submitInfoBuilder) const;

  void setCurrentFrame(uint8_t frame);

  const Swapchain& getSwapchain() const noexcept;

  uint32_t acquireNextImage();

  void present() const;

private:
  Swapchain _swapchain;

  uint8_t _currentFrame = 0;
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> _imageAvailableSemaphores;
  uint32_t _imageIndex = 0;
  lib::Buffer<VkSemaphore> _renderFinishedSemaphores;
};

}  // namespace vlkn
