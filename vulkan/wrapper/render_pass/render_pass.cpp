#include "render_pass.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/logical_device/resource_destroyer.h"
#include "vulkan/wrapper/render_pass/attachment_layout.h"
#include "vulkan/wrapper/util/check.h"

namespace {

template <typename T>
void chainExtendedField(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

RenderpassBuilder::RenderpassBuilder(const AttachmentLayout& attachmentLayout)
  : _attachmentLayout(attachmentLayout) {}

RenderpassBuilder& RenderpassBuilder::addDependency(
    uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask,
    VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
  _subpassDepencies.push_back(VkSubpassDependency2{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
    .srcSubpass = srcSubpassIndex,
    .dstSubpass = dstSubpassIndex,
    .srcStageMask = srcStageMask,
    .dstStageMask = dstStageMask,
    .srcAccessMask = srcAccessMask,
    .dstAccessMask = dstAccessMask});
  return *this;
}

RenderpassBuilder::Subpass& RenderpassBuilder::createSubpass() {
  return _subpasses.emplace_back(_attachmentLayout);
}

RenderpassBuilder& RenderpassBuilder::withMultiView(
    std::span<const uint32_t> viewMask, std::span<const uint32_t> correlationMask) {
  _multiViewInfo = MultiViewInfo{
    .viewMasks = std::vector<uint32_t>(std::cbegin(viewMask), std::cend(viewMask)),
    .correlationMasks =
        std::vector<uint32_t>(std::cbegin(correlationMask), std::cend(correlationMask))};
  return *this;
}

RenderpassBuilder& RenderpassBuilder::withMultiView(
    std::initializer_list<uint32_t> viewMask, std::initializer_list<uint32_t> correlationMask) {
  _multiViewInfo = MultiViewInfo{.viewMasks = viewMask, .correlationMasks = correlationMask};
  return *this;
}

RenderpassBuilder::Subpass::Subpass(const AttachmentLayout& attachmentLayout) noexcept
  : _attachmentLayout(attachmentLayout) {}

RenderpassBuilder::Subpass& RenderpassBuilder::Subpass::addOutputAttachment(
    uint32_t attachmentBinding) {
  const VkAttachmentReference2 attachmentRef{
    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
    .attachment = attachmentBinding,
    .layout = _attachmentLayout.getAttachmentVkImageLayout(attachmentBinding),
    .aspectMask = _attachmentLayout.getAttachmentAspectFlags(attachmentBinding)};
  switch (_attachmentLayout.getAttachmentType(attachmentBinding)) {
    case AttachmentType::COLOR:
      _colorAttachmentRefs.push_back(attachmentRef);
      break;
    case AttachmentType::COLOR_RESOLVE:
      _colorAttachmentResolveRefs.push_back(attachmentRef);
      break;
    case AttachmentType::DEPTH:
      _depthAttachmentRef.emplace(attachmentRef);
      break;
    default:
      throw EngineException("Failed to recognize attachment type.");
  }
  return *this;
}

RenderpassBuilder::Subpass& RenderpassBuilder::Subpass::addInputAttachment(
    uint32_t attachmentBinding) {
  _inputAttachmentRefs.push_back(VkAttachmentReference2{
    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
    .attachment = attachmentBinding,
    .layout = _attachmentLayout.getAttachmentVkImageLayout(attachmentBinding),
    .aspectMask = _attachmentLayout.getAttachmentAspectFlags(attachmentBinding)});
  return *this;
}

RenderpassBuilder::Subpass& RenderpassBuilder::Subpass::withShadingRateAttachment(
    uint32_t texelWidth, uint32_t texelHeight) {
  for (uint32_t binding = 0; binding < _attachmentLayout.getAttachmentsCount(); binding++) {
    if (_attachmentLayout.getAttachmentType(binding) == AttachmentType::FRAGMENT_SHADING_RATE) {
      return withShadingRateAttachment(binding, texelWidth, texelHeight);
    }
  }

  throw EngineException(
      "The attachment layout does not contain a fragment shading rate attachment.");
}

RenderpassBuilder::Subpass& RenderpassBuilder::Subpass::withShadingRateAttachment(
    uint32_t binding, uint32_t texelWidth, uint32_t texelHeight) {
  if (_attachmentLayout.getAttachmentType(binding) != AttachmentType::FRAGMENT_SHADING_RATE) {
    throw EngineException(
        "The specified attachment binding does not correspond to a fragment shading rate "
        "attachment.");
  }

  _fragmentShadingRateAttachmentRef = VkAttachmentReference2{
    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
    .attachment = binding,
    .layout = _attachmentLayout.getAttachmentVkImageLayout(binding)};

  _shadingRateAttachmentInfo = VkFragmentShadingRateAttachmentInfoKHR{
    .sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR,
    .pFragmentShadingRateAttachment = &_fragmentShadingRateAttachmentRef,
    .shadingRateAttachmentTexelSize = VkExtent2D{texelWidth, texelHeight}
  };

  chainExtendedField(&_pNext, _shadingRateAttachmentInfo);
  return *this;
}

VkSubpassDescription2 RenderpassBuilder::Subpass::getVkSubpassDescription(uint32_t viewMask) const {
  return VkSubpassDescription2{
    .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
    .pNext = _pNext,
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .viewMask = viewMask,
    .inputAttachmentCount = static_cast<uint32_t>(_inputAttachmentRefs.size()),
    .pInputAttachments = !_inputAttachmentRefs.empty() ? _inputAttachmentRefs.data() : nullptr,
    .colorAttachmentCount = static_cast<uint32_t>(_colorAttachmentRefs.size()),
    .pColorAttachments = !_colorAttachmentRefs.empty() ? _colorAttachmentRefs.data() : nullptr,
    .pResolveAttachments =
        !_colorAttachmentResolveRefs.empty() ? _colorAttachmentResolveRefs.data() : nullptr,
    .pDepthStencilAttachment =
        _depthAttachmentRef.has_value() ? &_depthAttachmentRef.value() : nullptr};
}

Renderpass RenderpassBuilder::build(
    const LogicalDevice& logicalDevice, VkRenderPassCreateFlags flags) {
  std::span<const VkAttachmentDescription2> attachmentDescriptions =
      _attachmentLayout.getVkAttachmentDescriptions();
  if (_multiViewInfo.has_value() && _multiViewInfo->viewMasks.size() != _subpasses.size()) {
    throw EngineException(
        "The number of view masks must be the same as the number of subpasses when using multiview "
        "feature.");
  }

  _subpassDescriptions = lib::Buffer<VkSubpassDescription2>(_subpasses.size());
  for (int i = 0; i < _subpasses.size(); i++) {
    _subpassDescriptions[i] = _subpasses[i].getVkSubpassDescription(
        _multiViewInfo.has_value() ? _multiViewInfo->viewMasks[i] : 0);
  }

  std::span<const AttachmentType> attachmentTypes = _attachmentLayout.getAttachmentTypes();
  auto it = std::find(std::cbegin(attachmentTypes), std::cend(attachmentTypes),
                      AttachmentType::FRAGMENT_DENSITY_MAP);
  if (it != std::cend(attachmentTypes)) {
    const uint32_t attachment = std::distance(std::cbegin(attachmentTypes), it);
    _fragmentDensityMapCreateInfo = VkRenderPassFragmentDensityMapCreateInfoEXT{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT,
      .fragmentDensityMapAttachment = VkAttachmentReference{
                                                            attachment, _attachmentLayout.getAttachmentVkImageLayout(attachment)}
    };
    chainExtendedField(&_pNext, *_fragmentDensityMapCreateInfo);
  }

  const VkRenderPassCreateInfo2 renderPassCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
    .pNext = _pNext,
    .flags = _flags = flags,
    .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
    .pAttachments = !attachmentDescriptions.empty() ? attachmentDescriptions.data() : nullptr,
    .subpassCount = static_cast<uint32_t>(_subpassDescriptions.size()),
    .pSubpasses = !_subpassDescriptions.empty() ? _subpassDescriptions.data() : nullptr,
    .dependencyCount = static_cast<uint32_t>(_subpassDepencies.size()),
    .pDependencies = !_subpassDepencies.empty() ? _subpassDepencies.data() : nullptr,
    .correlatedViewMaskCount = static_cast<uint32_t>(
        _multiViewInfo.has_value() ? _multiViewInfo->correlationMasks.size() : 0),
    .pCorrelatedViewMasks =
        _multiViewInfo.has_value() ? _multiViewInfo->correlationMasks.data() : nullptr};
  return Renderpass::create(logicalDevice, renderPassCreateInfo);
}

Renderpass::Renderpass(const LogicalDevice& logicalDeivce, VkRenderPass renderpass) noexcept
  : _logicalDevice(&logicalDeivce), _renderpass(renderpass) {}

Renderpass Renderpass::create(
    const LogicalDevice& logicalDevice, const VkRenderPassCreateInfo2& createInfo) {
  VkRenderPass renderpass;
  CHECK_VKCMD(vkCreateRenderPass2(logicalDevice.getVkDevice(), &createInfo, nullptr, &renderpass),
              "Failed to create VkRenderPass.");
  return Renderpass(logicalDevice, renderpass);
}

Renderpass::Renderpass(Renderpass&& renderpass) noexcept
  : _renderpass(std::exchange(renderpass._renderpass, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(renderpass._logicalDevice, nullptr)) {}

Renderpass& Renderpass::operator=(Renderpass&& renderpass) noexcept {
  if (this == &renderpass) [[unlikely]] {
    return *this;
  }

  destroy();

  _renderpass = std::exchange(renderpass._renderpass, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(renderpass._logicalDevice, nullptr);
  return *this;
}

Renderpass::~Renderpass() {
  destroy();
}

void Renderpass::destroy() {
  if (_renderpass != VK_NULL_HANDLE) {
    _logicalDevice->destroyResource([renderpass = _renderpass](DestroyerContext context) {
      vkDestroyRenderPass(context.device, renderpass, context.allocationCallbacks);
    });
  }
}

VkRenderPass Renderpass::getVkRenderPass() const noexcept {
  return _renderpass;
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
  return *_logicalDevice;
}
