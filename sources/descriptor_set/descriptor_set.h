#pragma once

#include "lib/buffer/buffer.h"
#include "lib/status/status.h"

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

	std::vector<uint32_t> _dynamicBuffersBaseSizes;
	std::shared_ptr<const DescriptorPool> _descriptorPool;

	DescriptorSet(const VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool);

public:
	DescriptorSet();

	DescriptorSet(DescriptorSet&& descriptorSet) noexcept;

	DescriptorSet& operator=(DescriptorSet&& DescriptorSet) noexcept;

	~DescriptorSet() = default;

	static lib::ErrorOr<DescriptorSet> create(const std::shared_ptr<const DescriptorPool>& descriptorPool);

	static lib::ErrorOr<std::vector<DescriptorSet>> create(const std::shared_ptr<const DescriptorPool>& descriptorPool, uint32_t numSets);

	void writeDescriptorSet(std::initializer_list<UniformBuffer*> uniformBuffers);

	void writeDescriptorSet(std::span<const UniformBuffer*> uniformBuffers);

	void bind(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides = {});

private:
	void writeDescriptorSetImpl(std::span<const UniformBuffer* const> uniformBuffers);
};