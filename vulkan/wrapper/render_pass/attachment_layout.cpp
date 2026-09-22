#include "attachment_layout.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "common/util/engine_exception.h"
#include "vulkan/wrapper/util/util.h"

AttachmentLayout::AttachmentLayout(VkSampleCountFlagBits numMsaaSamples)
  : _numMsaaSamples(numMsaaSamples) {}

VkSampleCountFlagBits AttachmentLayout::getNumMsaaSamples() const noexcept {
  return _numMsaaSamples;
}

std::span<const VkClearValue> AttachmentLayout::getVkClearValues() const noexcept {
  return _clearValues;
}

std::span<const VkAttachmentDescription2>
AttachmentLayout::getVkAttachmentDescriptions() const noexcept {
  return _attachmentDescriptions;
}

VkImageLayout AttachmentLayout::getAttachmentVkImageLayout(uint32_t index) const {
  if (_attachmentImageLayouts.size() <= index) [[unlikely]] {
    throw EngineException("Attachment index cannot exceed number of attachments.");
  }
  return _attachmentImageLayouts[index];
}

std::span<const VkImageLayout> AttachmentLayout::getAttachmentVkImageLayouts() const noexcept {
  return _attachmentImageLayouts;
}

AttachmentType AttachmentLayout::getAttachmentType(uint32_t index) const {
  if (_attachmentTypes.size() <= index) [[unlikely]] {
    throw EngineException("Attachment index cannot exceed number of attachments.");
  }
  return _attachmentTypes[index];
}

std::span<const AttachmentType> AttachmentLayout::getAttachmentTypes() const noexcept {
  return _attachmentTypes;
}

VkImageAspectFlags AttachmentLayout::getAttachmentAspectFlags(uint32_t index) const {
  if (_aspectFlags.size() <= index) [[unlikely]] {
    throw EngineException("Attachment index cannot exceed number of attachments.");
  }
  return _aspectFlags[index];
}

std::span<const VkImageAspectFlags> AttachmentLayout::getAttachmentAspectFlags() const noexcept {
  return _aspectFlags;
}

size_t AttachmentLayout::getAttachmentsCount() const noexcept {
  return _attachmentDescriptions.size();
}

uint32_t AttachmentLayout::getColorAttachmentsCount() const noexcept {
  return std::count(_attachmentTypes.cbegin(), _attachmentTypes.cend(), AttachmentType::COLOR);
}

namespace {

VkAttachmentDescription2 createDescription(
    VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp, VkImageLayout initialLayout, VkImageLayout finalLayout,
    VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE) noexcept {
  return VkAttachmentDescription2{
    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
    .flags = 0,
    .format = format,
    .samples = samples,
    .loadOp = loadOp,
    .storeOp = storeOp,
    .stencilLoadOp = stencilLoadOp,
    .stencilStoreOp = stencilStoreOp,
    .initialLayout = initialLayout,
    .finalLayout = finalLayout};
}

}  // namespace

AttachmentLayout& AttachmentLayout::addColorAttachment(
    VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
  _clearValues.push_back(VkClearValue{
    .color = {0.0f, 0.0f, 0.0f, 1.0f}
  });
  _attachmentDescriptions.push_back(
      createDescription(format, _numMsaaSamples, loadOp, storeOp, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::COLOR);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addColorPresentAttachment(
    VkFormat format, VkAttachmentLoadOp loadOp) {
  _clearValues.push_back(VkClearValue{
    .color = {0.0f, 0.0f, 0.0f, 1.0f}
  });
  _attachmentDescriptions.push_back(
      createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, VK_ATTACHMENT_STORE_OP_STORE,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::COLOR);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addDepthAttachment(
    VkFormat format, VkAttachmentStoreOp storeOp, VkAttachmentLoadOp stencilLoadOp,
    VkAttachmentStoreOp stencilStoreOp) {
  _clearValues.push_back(VkClearValue{
    .depthStencil = {1.0f, 0}
  });
  _attachmentDescriptions.push_back(createDescription(
      format, _numMsaaSamples, VK_ATTACHMENT_LOAD_OP_CLEAR, storeOp, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, stencilLoadOp, stencilStoreOp));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::DEPTH);
  _aspectFlags.push_back(
      hasStencil(format) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
                           VK_IMAGE_ASPECT_DEPTH_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addShadowAttachment(
    VkFormat format, VkImageLayout finalLayout) {
  _clearValues.push_back(VkClearValue{
    .depthStencil = {1.0f, 0}
  });
  _attachmentDescriptions.push_back(
      createDescription(format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::DEPTH);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_DEPTH_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addColorResolveAttachment(
    VkFormat format, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
  _clearValues.push_back(VkClearValue{
    .color = {0.0f, 0.0f, 0.0f, 1.0f}
  });
  _attachmentDescriptions.push_back(
      createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, storeOp, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::COLOR_RESOLVE);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addColorResolvePresentAttachment(
    VkFormat format, VkAttachmentLoadOp loadOp) {
  _clearValues.push_back(VkClearValue{
    .color = {0.0f, 0.0f, 0.0f, 1.0f}
  });
  _attachmentDescriptions.push_back(
      createDescription(format, VK_SAMPLE_COUNT_1_BIT, loadOp, VK_ATTACHMENT_STORE_OP_STORE,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  _attachmentTypes.push_back(AttachmentType::COLOR_RESOLVE);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addFragmentShadingRateAttachment() {
  _clearValues.push_back(VkClearValue{
    .color = {0.0f, 0.0f, 0.0f, 0.0f}
  });
  _attachmentDescriptions.push_back(createDescription(
      VK_FORMAT_R8_UINT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, VK_IMAGE_LAYOUT_GENERAL,
      VK_ATTACHMENT_LOAD_OP_CLEAR));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR);
  _attachmentTypes.push_back(AttachmentType::FRAGMENT_SHADING_RATE);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}

AttachmentLayout& AttachmentLayout::addFragmentDensityMapAttachment() {
  _attachmentDescriptions.push_back(createDescription(
      VK_FORMAT_R8G8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD,
      VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
      VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT));
  _attachmentImageLayouts.push_back(VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT);
  _attachmentTypes.push_back(AttachmentType::FRAGMENT_DENSITY_MAP);
  _aspectFlags.push_back(VK_IMAGE_ASPECT_COLOR_BIT);
  return *this;
}
