#include "render_pass.h"

#include <algorithm>
#include <iterator>

#include "vulkan_wrapper/render_pass/attachment_layout.h"
#include "vulkan_wrapper/util/check.h"

RenderpassBuilder::RenderpassBuilder(const AttachmentLayout& attachmentLayout)
  : _attachmentLayout(attachmentLayout) {}

RenderpassBuilder& RenderpassBuilder::addDependency(
    uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask,
    VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
  _subpassDepencies.emplace_back(
      srcSubpassIndex, dstSubpassIndex, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask);
  return *this;
}

RenderpassBuilder& RenderpassBuilder::addSubpass(std::initializer_list<uint8_t> outputAttachments,
                                                 std::initializer_list<uint8_t> inputAttachments) {
  Subpass subpass;
  for (uint8_t index : outputAttachments) {
    UPDATE_STATUS(_status, subpass.addOutputAttachment(_attachmentLayout, index));
  }
  for (uint8_t index : inputAttachments) {
    // TODO set proper image layouts
    UPDATE_STATUS(
        _status, subpass.addInputAttachment(_attachmentLayout, index, VK_IMAGE_LAYOUT_GENERAL));
  }
  _subpasses.push_back(subpass);
  return *this;
}

RenderpassBuilder& RenderpassBuilder::withMultiView(
    std::initializer_list<uint32_t> viewMask, std::initializer_list<uint32_t> correlationMask) {}

Status RenderpassBuilder::Subpass::addOutputAttachment(
    const AttachmentLayout& layout, uint32_t attachmentBinding) {
  std::span<const AttachmentType> attachmentTypes = layout.getAttachmentsTypes();
  if (attachmentTypes.size() <= attachmentBinding) {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }

  switch (attachmentTypes[attachmentBinding]) {
    case AttachmentType::COLOR:
      _colorAttachmentRefs.emplace_back(
          attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
      break;
    case AttachmentType::COLOR_RESOLVE:
      _colorAttachmentResolveRefs.emplace_back(
          attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
      break;
    case AttachmentType::DEPTH:
      _depthAttachmentRefs.emplace_back(
          attachmentBinding, layout.getVkSubpassLayouts()[attachmentBinding]);
      break;
    default:
      return Error(EngineError::NOT_RECOGNIZED_TYPE);
  }
  return StatusOk();
}

Status RenderpassBuilder::Subpass::addInputAttachment(
    const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout) {
  if (layout.getAttachmentsTypes().size() <= attachmentBinding) {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  _inputAttachmentRefs.emplace_back(attachmentBinding, imageLayout);
  return StatusOk();
}

VkSubpassDescription RenderpassBuilder::Subpass::getVkSubpassDescription() const {
  return VkSubpassDescription{
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .inputAttachmentCount = static_cast<uint32_t>(_inputAttachmentRefs.size()),
    .pInputAttachments = !_inputAttachmentRefs.empty() ? _inputAttachmentRefs.data() : nullptr,
    .colorAttachmentCount = static_cast<uint32_t>(_colorAttachmentRefs.size()),
    .pColorAttachments = !_colorAttachmentRefs.empty() ? _colorAttachmentRefs.data() : nullptr,
    .pResolveAttachments =
        !_colorAttachmentResolveRefs.empty() ? _colorAttachmentResolveRefs.data() : nullptr,
    .pDepthStencilAttachment =
        !_depthAttachmentRefs.empty() ? _depthAttachmentRefs.data() : nullptr};
}

ErrorOr<Renderpass> RenderpassBuilder::build(const LogicalDevice& logicalDevice) {
  RETURN_IF_ERROR(_status);
  std::span<const VkAttachmentDescription> attachmentDescriptions =
      _attachmentLayout.getVkAttachmentDescriptions();
  lib::Buffer<VkSubpassDescription> subpassDescriptions(_subpasses.size());
  std::transform(_subpasses.cbegin(), _subpasses.cend(), subpassDescriptions.begin(),
                 [](const Subpass& subpass) {
                   return subpass.getVkSubpassDescription();
                 });

  const VkRenderPassCreateInfo renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
    .pAttachments = attachmentDescriptions.data(),
    .subpassCount = static_cast<uint32_t>(subpassDescriptions.size()),
    .pSubpasses = subpassDescriptions.data(),
    .dependencyCount = static_cast<uint32_t>(_subpassDepencies.size()),
    .pDependencies = _subpassDepencies.data()};

  VkRenderPass renderpass;
  CHECK_VKCMD(
      vkCreateRenderPass(logicalDevice.getVkDevice(), &renderPassInfo, nullptr, &renderpass));
  return Renderpass(logicalDevice, renderpass, _attachmentLayout);
}

Renderpass::Renderpass(const LogicalDevice& logicalDeivce, VkRenderPass renderpass,
                       const AttachmentLayout& attachmentLayout)
  : _logicalDevice(&logicalDeivce), _renderpass(renderpass), _attachmentsLayout(attachmentLayout) {}

Renderpass::Renderpass(Renderpass&& renderpass) noexcept
  : _renderpass(std::exchange(renderpass._renderpass, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(renderpass._logicalDevice, nullptr)),
    _attachmentsLayout(std::move(renderpass._attachmentsLayout)) {}

Renderpass& Renderpass::operator=(Renderpass&& renderpass) noexcept {
  if (this == &renderpass) {
    return *this;
  }
  _renderpass = std::exchange(renderpass._renderpass, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(renderpass._logicalDevice, nullptr);
  _attachmentsLayout = std::move(renderpass._attachmentsLayout);
  return *this;
}

Renderpass::~Renderpass() {
  if (_renderpass != VK_NULL_HANDLE && _logicalDevice != nullptr) {
    vkDestroyRenderPass(_logicalDevice->getVkDevice(), _renderpass, nullptr);
  }
}

VkRenderPass Renderpass::getVkRenderPass() const {
  return _renderpass;
}

const AttachmentLayout& Renderpass::getAttachmentsLayout() const {
  return _attachmentsLayout;
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
  return *_logicalDevice;
}
