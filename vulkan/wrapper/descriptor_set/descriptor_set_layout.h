#pragma once

#include <span>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "vulkan/wrapper/logical_device/logical_device.h"

class DescriptorSetLayout {
  VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

  const LogicalDevice* _logicalDevice;

  DescriptorSetLayout(const LogicalDevice& logicalDevice, VkDescriptorSetLayout layout);

public:
  DescriptorSetLayout() = default;

  DescriptorSetLayout(DescriptorSetLayout&& layout) noexcept;

  DescriptorSetLayout& operator=(DescriptorSetLayout&& layout) noexcept;

  ~DescriptorSetLayout();

  static ErrorOr<DescriptorSetLayout> create(
      const LogicalDevice& logicalDevice, std::span<const VkDescriptorSetLayoutBinding> bindings,
      std::span<const VkDescriptorBindingFlags> bindingFlags = {},
      VkDescriptorSetLayoutCreateFlags flags = 0);

  VkDescriptorSetLayout getVkDescriptorSetLayout() const;
};
