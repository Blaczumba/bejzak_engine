#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/logical_device/logical_device.h"

#include <vulkan/vulkan.h>

#include <span>

class DescriptorSetLayout {
	VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

	const LogicalDevice* _logicalDevice;

	DescriptorSetLayout(const LogicalDevice& logicalDevice, VkDescriptorSetLayout layout);

public:
	DescriptorSetLayout(DescriptorSetLayout&& layout) noexcept;

	DescriptorSetLayout& operator=(DescriptorSetLayout&& layout) noexcept;

	~DescriptorSetLayout();

	static ErrorOr<DescriptorSetLayout> create(const LogicalDevice& logicalDevice, std::span<const VkDescriptorSetLayoutBinding> bindings, std::span<const VkDescriptorBindingFlags> bindingFlags = {}, VkDescriptorSetLayoutCreateFlags flags = 0);

	VkDescriptorSetLayout getVkDescriptorSetLayout() const;
};