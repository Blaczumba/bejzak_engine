#include "descriptor_pool.h"

#include "lib/buffer/buffer.h"
#include "logical_device/logical_device.h"
#include "descriptor_set.h"
#include "descriptor_set_layout.h"

DescriptorPool::DescriptorPool(const VkDescriptorPool descriptorPool, const LogicalDevice& logicalDevice, uint32_t maxNumSets)
	: _descriptorPool(descriptorPool), _logicalDevice(logicalDevice), _maxNumSets(maxNumSets), _allocatedSets(0) {}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(_logicalDevice.getVkDevice(), _descriptorPool, nullptr);
}

lib::ErrorOr<std::unique_ptr<DescriptorPool>> DescriptorPool::create(const LogicalDevice& logicalDevice, uint32_t maxNumSets) {
	static constexpr VkDescriptorPoolSize poolSizes[] = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
	};

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxNumSets,
		.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
		.pPoolSizes = poolSizes
	};

	VkDescriptorPool descriptorPool;
	if (vkCreateDescriptorPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		return lib::Error("Failed to create descriptor pool.");
	}
	return std::unique_ptr<DescriptorPool>(new DescriptorPool(descriptorPool, logicalDevice, maxNumSets));
}

const VkDescriptorPool DescriptorPool::getVkDescriptorPool() const {
	return _descriptorPool;
}

lib::ErrorOr<DescriptorSet> DescriptorPool::createDesriptorSet(const DescriptorSetLayout& layout) const {
	++_allocatedSets;
	if (_allocatedSets > _maxNumSets) {
		--_allocatedSets;
		return lib::Error("Exceeded the maximum number of allocated sets.");
	}
	return DescriptorSet::create(shared_from_this(), layout);
}

lib::ErrorOr<std::vector<DescriptorSet>> DescriptorPool::createDesriptorSets(const DescriptorSetLayout& layout, uint32_t numSets) const {
	_allocatedSets += numSets;
	if (_allocatedSets > _maxNumSets) {
		_allocatedSets -= numSets;
		return lib::Error("Exceeded the maximum number of allocated sets.");
	}
	return DescriptorSet::create(shared_from_this(), layout, numSets);
}

bool DescriptorPool::maxSetsReached() const {
	return _allocatedSets >= _maxNumSets;
}

const LogicalDevice& DescriptorPool::getLogicalDevice() const {
	return _logicalDevice;
}
