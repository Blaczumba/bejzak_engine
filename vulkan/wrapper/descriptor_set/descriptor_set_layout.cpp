#include "descriptor_set_layout.h"

#include <algorithm>
#include <cstdint>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/util/check.h"

DescriptorSetLayout::DescriptorSetLayout(
    const LogicalDevice& logicalDevice, VkDescriptorSetLayout layout) noexcept
  : _logicalDevice(&logicalDevice), _descriptorSetLayout(layout) {}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& layout) noexcept
  : _descriptorSetLayout(std::exchange(layout._descriptorSetLayout, VK_NULL_HANDLE)),
    _logicalDevice(layout._logicalDevice) {}

void DescriptorSetLayout::destroy() {
  if (_descriptorSetLayout != VK_NULL_HANDLE) {
    _logicalDevice->destroyResource(
        [descriptorSetlayout = _descriptorSetLayout](DestroyerContext context) {
          vkDestroyDescriptorSetLayout(
              context.device, descriptorSetlayout, context.allocationCallbacks);
        });
  }
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& layout) noexcept {
  if (&layout == this) {
    return *this;
  }

  destroy();

  _descriptorSetLayout = std::exchange(layout._descriptorSetLayout, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(layout._logicalDevice, nullptr);
  return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() {
  destroy();
}

DescriptorSetLayout DescriptorSetLayout::create(
    const LogicalDevice& logicalDevice, const VkDescriptorSetLayoutCreateInfo& createInfo) {
  VkDescriptorSetLayout descriptorSetLayout;
  CHECK_VKCMD(vkCreateDescriptorSetLayout(
                  logicalDevice.getVkDevice(), &createInfo, nullptr, &descriptorSetLayout),
              "Failed to create VkDescriptorSetLayout.");
  return DescriptorSetLayout(logicalDevice, descriptorSetLayout);
}

VkDescriptorSetLayout DescriptorSetLayout::getVkDescriptorSetLayout() const noexcept {
  return _descriptorSetLayout;
}

DescriptorSetLayoutBuilder&& DescriptorSetLayoutBuilder::addBinding(
    uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VkShaderStageFlags stageFlags, VkDescriptorBindingFlags bindingFlags,
    const VkSampler* immutableSamplers) && {
  const VkDescriptorSetLayoutBinding newBinding{
    .binding = binding,
    .descriptorType = descriptorType,
    .descriptorCount = descriptorCount,
    .stageFlags = stageFlags,
    .pImmutableSamplers = immutableSamplers};

  for (size_t i = 0; i < _bindings.size(); ++i) {
    if (_bindings[i].binding == binding) {
      _bindings[i] = newBinding;
      _bindingFlags[i] = bindingFlags;
      return std::move(*this);
    }
  }

  _bindings.push_back(newBinding);
  _bindingFlags.push_back(bindingFlags);
  return std::move(*this);
}

DescriptorSetLayoutBuilder&& DescriptorSetLayoutBuilder::withFlags(
    VkDescriptorSetLayoutCreateFlags flags) && noexcept {
  _flags = flags;
  return std::move(*this);
}

DescriptorSetLayoutMetadata DescriptorSetLayoutBuilder::buildMetadata() const noexcept {
  DescriptorSetLayoutMetadata metadata = {
    .bindings = lib::Buffer<std::pair<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>>(
        _bindings.size()),
    .flags = _flags};
  std::transform(
      _bindings.cbegin(), _bindings.cend(), _bindingFlags.cbegin(), metadata.bindings.begin(),
      [](const VkDescriptorSetLayoutBinding& layoutBinding, VkDescriptorBindingFlags bindingFlag) {
        return std::make_pair(layoutBinding, bindingFlag);
      });
  return metadata;
}

DescriptorSetLayout DescriptorSetLayoutBuilder::build(const LogicalDevice& logicalDevice) const {
  const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(_bindingFlags.size()),
    .pBindingFlags = _bindingFlags.data()};
  const VkDescriptorSetLayoutCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = &bindingFlagsCreateInfo,
    .flags = _flags,
    .bindingCount = static_cast<uint32_t>(_bindings.size()),
    .pBindings = _bindings.data()};
  return DescriptorSetLayout::create(logicalDevice, createInfo);
}

std::tuple<DescriptorSetLayout, DescriptorSetLayoutMetadata>
DescriptorSetLayoutBuilder::buildWithMetadata(const LogicalDevice& logicalDevice) const {
  return std::make_tuple(build(logicalDevice), buildMetadata());
}
