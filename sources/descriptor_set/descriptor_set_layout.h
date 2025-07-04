#pragma once

#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

class LogicalDevice;

class DescriptorSetLayout {
	VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> _bindings;
	std::vector<VkDescriptorBindingFlags> _bindingFlags;
	uint8_t _binding;

	const LogicalDevice* _logicalDevice;

public:
	DescriptorSetLayout(const LogicalDevice& logicalDevice);

	DescriptorSetLayout(DescriptorSetLayout&& layout) noexcept;

	DescriptorSetLayout& operator=(DescriptorSetLayout&& layout) noexcept;

	~DescriptorSetLayout();

	Status build(VkDescriptorSetLayoutCreateFlags flags = {});

	void addLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1, VkDescriptorBindingFlags bindingFlags = {}, const VkSampler* pImmutableSamplers = nullptr);

	const VkDescriptorSetLayout& getVkDescriptorSetLayout() const;

};