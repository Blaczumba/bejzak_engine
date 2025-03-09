#pragma once

#include "lib/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class DescriptorPool;
class LogicalDevice;
class UniformBuffer;
class Pipeline;

class DescriptorSet {
	VkDescriptorSet _descriptorSet;

	std::vector<uint32_t> _dynamicBuffersBaseSizes;
	const std::shared_ptr<const DescriptorPool> _descriptorPool;

	DescriptorSet(const VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool);

public:
	~DescriptorSet() = default;

	static lib::ErrorOr<std::unique_ptr<DescriptorSet>> create(const std::shared_ptr<const DescriptorPool>& descriptorPool);

	void updateDescriptorSet(std::initializer_list<UniformBuffer*> uniformBuffers);
	void bind(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides = {});

	const VkDescriptorSet getVkDescriptorSet(size_t i) const;

};