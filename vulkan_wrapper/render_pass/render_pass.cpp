#include "render_pass.h"

#include <algorithm>
#include <iterator>

#include "vulkan_wrapper/render_pass/attachment_layout.h"
#include "vulkan_wrapper/util/check.h"

Status Renderpass::Subpass::addOutputAttachment(
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

Status Renderpass::Subpass::addInputAttachment(
    const AttachmentLayout& layout, uint32_t attachmentBinding, VkImageLayout imageLayout) {
  if (layout.getAttachmentsTypes().size() <= attachmentBinding) {
    return Error(EngineError::INDEX_OUT_OF_RANGE);
  }
  _inputAttachmentRefs.emplace_back(attachmentBinding, imageLayout);
  return StatusOk();
}

VkSubpassDescription Renderpass::Subpass::getVkSubpassDescription() const {
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

Renderpass::Renderpass(const LogicalDevice& logicalDevice, const AttachmentLayout& layout)
  : _logicalDevice(&logicalDevice), _attachmentsLayout(layout) {}

Renderpass::Renderpass(Renderpass&& renderpass) noexcept
  : _renderpass(std::exchange(renderpass._renderpass, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(renderpass._logicalDevice, nullptr)),
    _attachmentsLayout(std::move(renderpass._attachmentsLayout)),
    _subpasses(std::move(renderpass._subpasses)),
    _subpassDepencies(std::move(renderpass._subpassDepencies)) {}

Renderpass& Renderpass::operator=(Renderpass&& renderpass) noexcept {
  if (this == &renderpass) {
    return *this;
  }
  _renderpass = std::exchange(renderpass._renderpass, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(renderpass._logicalDevice, nullptr);
  _attachmentsLayout = std::move(renderpass._attachmentsLayout);
  _subpasses = std::move(renderpass._subpasses);
  _subpassDepencies = std::move(renderpass._subpassDepencies);
  return *this;
}

Status Renderpass::build() {
  if (_renderpass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(_logicalDevice->getVkDevice(), _renderpass, nullptr);
  }

  std::span<const VkAttachmentDescription> attachmentDescriptions =
      _attachmentsLayout.getVkAttachmentDescriptions();
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

  CHECK_VKCMD(
      vkCreateRenderPass(_logicalDevice->getVkDevice(), &renderPassInfo, nullptr, &_renderpass));
  return StatusOk();
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

Status Renderpass::addSubpass(std::initializer_list<uint8_t> outputAttachments,
                              std::initializer_list<uint8_t> inputAttachments) {
  Subpass subpass;
  for (uint8_t index : outputAttachments) {
    RETURN_IF_ERROR(subpass.addOutputAttachment(_attachmentsLayout, index));
  }
  for (uint8_t index : inputAttachments) {
    // TODO set proper image layouts
    RETURN_IF_ERROR(subpass.addInputAttachment(_attachmentsLayout, index, VK_IMAGE_LAYOUT_GENERAL));
  }
  _subpasses.push_back(subpass);
  return StatusOk();
}

void Renderpass::addDependency(
    uint32_t srcSubpassIndex, uint32_t dstSubpassIndex, VkPipelineStageFlags srcStageMask,
    VkAccessFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
  _subpassDepencies.emplace_back(
      srcSubpassIndex, dstSubpassIndex, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask);
}

const LogicalDevice& Renderpass::getLogicalDevice() const {
  return *_logicalDevice;
}
