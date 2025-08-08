#include "descriptor_pool.h"
#include "descriptor_set.h"
#include "descriptor_set_layout.h"

#include "common/status/status.h"
#include "lib/buffer/buffer.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/util/check.h"

DescriptorPool::DescriptorPool(VkDescriptorPool descriptorPool, const LogicalDevice& logicalDevice, uint32_t maxNumSets)
	: _descriptorPool(descriptorPool), _logicalDevice(logicalDevice), _maxNumSets(maxNumSets), _allocatedSets(0) {}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(_logicalDevice.getVkDevice(), _descriptorPool, nullptr);
}

ErrorOr<std::unique_ptr<DescriptorPool>> DescriptorPool::create(const LogicalDevice& logicalDevice, uint32_t maxNumSets, VkDescriptorPoolCreateFlags flags) {
	static constexpr VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
	};

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = flags,
		.maxSets = maxNumSets,
		.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
		.pPoolSizes = poolSizes
	};

	VkDescriptorPool descriptorPool;
	CHECK_VKCMD(vkCreateDescriptorPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &descriptorPool));
	return std::unique_ptr<DescriptorPool>(new DescriptorPool(descriptorPool, logicalDevice, maxNumSets));
}

VkDescriptorPool DescriptorPool::getVkDescriptorPool() const {
	return _descriptorPool;
}

ErrorOr<DescriptorSet> DescriptorPool::createDesriptorSet(VkDescriptorSetLayout layout) const {
	++_allocatedSets;
	if (_allocatedSets > _maxNumSets) {
		--_allocatedSets;
		return Error(EngineError::RESOURCE_EXHAUSTED);
	}
	return DescriptorSet::create(shared_from_this(), layout);
}

ErrorOr<std::vector<DescriptorSet>> DescriptorPool::createDesriptorSets(VkDescriptorSetLayout layout, uint32_t numSets) const {
	_allocatedSets += numSets;
	if (_allocatedSets > _maxNumSets) {
		_allocatedSets -= numSets;
		return Error(EngineError::RESOURCE_EXHAUSTED);
	}
	return DescriptorSet::create(shared_from_this(), layout, numSets);
}

bool DescriptorPool::maxSetsReached() const {
	return _allocatedSets >= _maxNumSets;
}

const LogicalDevice& DescriptorPool::getLogicalDevice() const {
	return _logicalDevice;
}
