#pragma once

#include "vulkan_wrapper/descriptor_set/descriptor_set_layout.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <span>
#include <vector>

class DescriptorPool;
class LogicalDevice;
class UniformBuffer;
class Pipeline;

class DescriptorSet {
	VkDescriptorSet _descriptorSet;

	std::shared_ptr<const DescriptorPool> _descriptorPool;

	DescriptorSet(VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool);

public:
	DescriptorSet() = default;

	DescriptorSet(DescriptorSet&& descriptorSet) noexcept;

	DescriptorSet& operator=(DescriptorSet&& DescriptorSet) noexcept;

	~DescriptorSet() = default;

	static ErrorOr<DescriptorSet> create(const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout);

	static ErrorOr<std::vector<DescriptorSet>> create(const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout, uint32_t numSets);

	VkDescriptorSet getVkDescriptorSet() const;

	const DescriptorPool& getDescriptorPool() const;
};