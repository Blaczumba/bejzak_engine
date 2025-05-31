#include "descriptor_pool.h"

#include "lib/buffer/buffer.h"
#include "logical_device/logical_device.h"
#include "descriptor_set.h"
#include "descriptor_set_layout.h"

DescriptorPool::DescriptorPool(const VkDescriptorPool descriptorPool, const LogicalDevice& logicalDevice, const DescriptorSetLayout& descriptorSetLayout, uint32_t maxNumSets)
	: _descriptorPool(descriptorPool), _logicalDevice(logicalDevice), _descriptorSetLayout(descriptorSetLayout), _maxNumSets(maxNumSets), _allocatedSets(0) {}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(_logicalDevice.getVkDevice(), _descriptorPool, nullptr);
}

lib::ErrorOr<std::unique_ptr<DescriptorPool>> DescriptorPool::create(const LogicalDevice& logicalDevice, const DescriptorSetLayout& descriptorSetLayout, uint32_t maxNumSets) {
	const auto& descriptorDict = descriptorSetLayout.getDescriptorTypeCounter();
	lib::Buffer<VkDescriptorPoolSize> poolSizes(descriptorDict.size());
	std::transform(descriptorDict.cbegin(), descriptorDict.cend(), poolSizes.begin(), [](std::pair<VkDescriptorType, uint32_t> count) { return VkDescriptorPoolSize{ count.first, count.second }; });

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxNumSets,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VkDescriptorPool descriptorPool;
	if (vkCreateDescriptorPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		return lib::Error("Failed to create descriptor pool.");
	}
	return std::unique_ptr<DescriptorPool>(new DescriptorPool(descriptorPool, logicalDevice, descriptorSetLayout, maxNumSets));
}

const VkDescriptorPool DescriptorPool::getVkDescriptorPool() const {
	return _descriptorPool;
}

const DescriptorSetLayout& DescriptorPool::getDescriptorSetLayout() const {
	return _descriptorSetLayout;
}

lib::ErrorOr<DescriptorSet> DescriptorPool::createDesriptorSet() const {
	++_allocatedSets;
	if (_allocatedSets > _maxNumSets) {
		--_allocatedSets;
		return lib::Error("Exceeded the maximum number of allocated sets.");
	}
	return DescriptorSet::create(shared_from_this());
}

lib::ErrorOr<std::vector<DescriptorSet>> DescriptorPool::createDesriptorSets(uint32_t numSets) const {
	_allocatedSets += numSets;
	if (_allocatedSets > _maxNumSets) {
		_allocatedSets -= numSets;
		return lib::Error("Exceeded the maximum number of allocated sets.");
	}
	return DescriptorSet::create(shared_from_this(), numSets);
}

bool DescriptorPool::maxSetsReached() const {
	return _allocatedSets >= _maxNumSets;
}

const LogicalDevice& DescriptorPool::getLogicalDevice() const {
	return _logicalDevice;
}
