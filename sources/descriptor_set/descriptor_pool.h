#pragma once

#include "lib/buffer/buffer.h"
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class LogicalDevice;
class DescriptorSetLayout;
class DescriptorSet;

class DescriptorPool : public std::enable_shared_from_this<const DescriptorPool> {
	VkDescriptorPool _descriptorPool;
	const uint32_t _maxNumSets;
	mutable uint32_t _allocatedSets;

	const LogicalDevice& _logicalDevice;

	DescriptorPool(const VkDescriptorPool descriptorPool, const LogicalDevice& logicalDevice, uint32_t maxNumSets);

public:
	~DescriptorPool();

	static ErrorOr<std::unique_ptr<DescriptorPool>> create(const LogicalDevice& logicalDevice, uint32_t maxNumSets, VkDescriptorPoolCreateFlags flags = {});

	const VkDescriptorPool getVkDescriptorPool() const;

	ErrorOr<DescriptorSet> createDesriptorSet(const VkDescriptorSetLayout layout) const;

	ErrorOr<std::vector<DescriptorSet>> createDesriptorSets(const VkDescriptorSetLayout layout, uint32_t numSets) const;

	bool maxSetsReached() const;

	const LogicalDevice& getLogicalDevice() const;
};