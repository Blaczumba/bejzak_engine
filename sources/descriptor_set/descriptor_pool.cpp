#include "descriptor_pool.h"

#include "logical_device/logical_device.h"
#include "descriptor_set.h"
#include "descriptor_set_layout.h"

DescriptorPool::DescriptorPool(const LogicalDevice& logicalDevice, const DescriptorSetLayout& descriptorSetLayout, uint32_t maxNumSets)
	: _logicalDevice(logicalDevice), _descriptorSetLayout(descriptorSetLayout), _maxNumSets(maxNumSets), _allocatedSets(0) {

	std::vector<VkDescriptorPoolSize> poolSizes;
	size_t count = _descriptorSetLayout.getDescriptorTypeCounter().size();
	poolSizes.reserve(count);
	for (const auto [descriptorType, numOccurances] : _descriptorSetLayout.getDescriptorTypeCounter()) {
		poolSizes.emplace_back(descriptorType, _maxNumSets * numOccurances);
	}

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = _maxNumSets,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	if (vkCreateDescriptorPool(_logicalDevice.getVkDevice(), &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(_logicalDevice.getVkDevice(), _descriptorPool, nullptr);
}

const VkDescriptorPool DescriptorPool::getVkDescriptorPool() const {
	return _descriptorPool;
}

const DescriptorSetLayout& DescriptorPool::getDescriptorSetLayout() const {
	return _descriptorSetLayout;
}

std::unique_ptr<DescriptorSet> DescriptorPool::createDesriptorSet() const {
	++_allocatedSets;
	return std::make_unique<DescriptorSet>(shared_from_this());
}

bool DescriptorPool::maxSetsReached() const {
	return _allocatedSets >= _maxNumSets;
}

const LogicalDevice& DescriptorPool::getLogicalDevice() const {
	return _logicalDevice;
}
