#pragma once

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

public:
	DescriptorSet(const std::shared_ptr<const DescriptorPool>& descriptorPool);
	~DescriptorSet() = default;

	void updateDescriptorSet(const std::vector<UniformBuffer*>& uniformBuffers);
	void bind(VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides = {});

	const VkDescriptorSet getVkDescriptorSet(size_t i) const;

};