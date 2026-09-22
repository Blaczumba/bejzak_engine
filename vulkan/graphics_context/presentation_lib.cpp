#include "presentation_lib.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>

#include "common/abstractions/contexts.h"
#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/builders/submit_info_builder.h"
#include "vulkan/wrapper/swapchain/swapchain.h"
#include "vulkan/wrapper/util/check.h"

namespace vlkn {

PresentationContext::PresentationContext(Swapchain&& swapchain) noexcept
  : _swapchain(std::move(swapchain)), _renderFinishedSemaphores(_swapchain.getImagesCount()) {
  static constexpr VkSemaphoreCreateInfo semaphoreInfo = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  const VkDevice device = _swapchain.getLogicalDevice().getVkDevice();
  for (VkSemaphore& semaphore : _imageAvailableSemaphores) {
    CHECK_VKCMD(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore),
                "Failed to create VkSemaphore.");
  }
  for (VkSemaphore& semaphore : _renderFinishedSemaphores) {
    CHECK_VKCMD(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore),
                "Failed to create VkSemaphore.");
  }
}

std::unique_ptr<PresentationContext> PresentationContext::create(Swapchain&& swapchain) {
  return std::unique_ptr<PresentationContext>(new PresentationContext(std::move(swapchain)));
}

PresentationContext::~PresentationContext() {
  const VkDevice device = _swapchain.getLogicalDevice().getVkDevice();
  vkDeviceWaitIdle(device);
  for (VkSemaphore& semaphore : _imageAvailableSemaphores) {
    vkDestroySemaphore(device, semaphore, nullptr);
  }
  for (VkSemaphore& semaphore : _renderFinishedSemaphores) {
    vkDestroySemaphore(device, semaphore, nullptr);
  }
}

common::PresentResources PresentationContext::getPresentResources() const {
  const auto [width, height] = _swapchain.getExtent();
  std::span<const VkImageView> imageViews = _swapchain.getImageViews();
  return common::PresentResources{
    .imageFormat = static_cast<int64_t>(_swapchain.getVkFormat()),
    .width = width,
    .height = height,
    .numLayers = 1,
    .imageViews =
        std::span(reinterpret_cast<const std::byte*>(imageViews.data()), imageViews.size()),
    .multiview = false,
  };
}

void PresentationContext::synchronizeSubmit(SubmitInfoBuilder* submitInfoBuilder) const {
  submitInfoBuilder
      ->withWaitSemaphores({_imageAvailableSemaphores[_currentFrame]},
                           {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT})
      .withSignalSemaphores({_renderFinishedSemaphores[_imageIndex]});
}

void PresentationContext::setCurrentFrame(uint8_t frame) {
  _currentFrame = frame;
}

const Swapchain& PresentationContext::getSwapchain() const noexcept {
  return _swapchain;
}

uint32_t PresentationContext::acquireNextImage() {
  CHECK_VKCMD(_swapchain.acquireNextImage(_imageAvailableSemaphores[_currentFrame], &_imageIndex),
              "Failed to acquire next image from swapchain.");
  return _imageIndex;
}

void PresentationContext::present() const {
  CHECK_VKCMD(_swapchain.present(_imageIndex, _renderFinishedSemaphores[_imageIndex]),
              "Failed to present swapchain image.");
}

}  // namespace vlkn
