#pragma once

#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

using DescriptorTypeCounterDict = std::unordered_map<VkDescriptorType, uint32_t>;

class LogicalDevice;

class DescriptorSetLayout {
	VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> _bindings;
	std::vector<VkDescriptorBindingFlags> _bindingFlags;
	DescriptorTypeCounterDict _descriptorTypeOccurances;
	uint32_t _binding;

	const LogicalDevice& _logicalDevice;

	DescriptorSetLayout(const LogicalDevice& logicalDevice);

public:
	~DescriptorSetLayout();

	static std::unique_ptr<DescriptorSetLayout> create(const LogicalDevice& logicalDevice);

	Status build(VkDescriptorSetLayoutCreateFlags flags = {});

	void addLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1, VkDescriptorBindingFlags bindingFlags = {}, const VkSampler* pImmutableSamplers = nullptr);

	const VkDescriptorSetLayout& getVkDescriptorSetLayout() const;

	const DescriptorTypeCounterDict& getDescriptorTypeCounter() const;
};