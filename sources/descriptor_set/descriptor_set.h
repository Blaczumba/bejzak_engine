#pragma once

#include "descriptor_set/descriptor_set_layout.h"
#include "lib/buffer/buffer.h"
#include "status/status.h"

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
	DescriptorSet() = default;

	DescriptorSet(DescriptorSet&& descriptorSet) noexcept;

	DescriptorSet& operator=(DescriptorSet&& DescriptorSet) noexcept;

	~DescriptorSet() = default;

	static ErrorOr<DescriptorSet> create(const std::shared_ptr<const DescriptorPool>& descriptorPool, const VkDescriptorSetLayout layout);

	static ErrorOr<std::vector<DescriptorSet>> create(const std::shared_ptr<const DescriptorPool>& descriptorPool, const VkDescriptorSetLayout layout, uint32_t numSets);

	void writeDescriptorSet(std::initializer_list<UniformBuffer*> uniformBuffers);

	void writeDescriptorSet(std::span<const UniformBuffer*> uniformBuffers);

	void bind(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, uint32_t firstSet = 0, std::initializer_list<uint32_t> dynamicOffsetStrides = {});

	const VkDescriptorSet getVkDescriptorSet() const;

	const DescriptorPool& getDescriptorPool() const;

private:
	void writeDescriptorSetImpl(std::span<const UniformBuffer* const> uniformBuffers);
};