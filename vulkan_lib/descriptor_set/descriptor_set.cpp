#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "vulkan_lib/lib/buffer/buffer.h"
#include "vulkan_lib/logical_device/logical_device.h"
#include "vulkan_lib/pipeline/pipeline.h"
#include "vulkan_lib/status/status.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

DescriptorSet::DescriptorSet(VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool)
	: _descriptorSet(descriptorSet), _descriptorPool(descriptorPool) {}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet) noexcept
    : _descriptorSet(descriptorSet._descriptorSet), _descriptorPool(std::move(descriptorSet._descriptorPool)) {

}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& descriptorSet) noexcept {
    if (this == &descriptorSet) {
        return *this;
    }
    _descriptorSet = descriptorSet._descriptorSet;
    _descriptorPool = std::move(descriptorSet._descriptorPool);

    return *this;
}

ErrorOr<DescriptorSet> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout) {
    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptorSet;
    if (VkResult result = vkAllocateDescriptorSets(descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, &descriptorSet); result != VK_SUCCESS) {
        return Error(result);
    }
    return DescriptorSet(descriptorSet, descriptorPool);
}

ErrorOr<std::vector<DescriptorSet>> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool, VkDescriptorSetLayout layout, uint32_t numSets) {
    const std::vector<VkDescriptorSetLayout> layouts(numSets, layout);
    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = numSets,
        .pSetLayouts = layouts.data(),
    };

    lib::Buffer<VkDescriptorSet> descriptorSets(numSets);
    if (VkResult result = vkAllocateDescriptorSets(descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, descriptorSets.data()); result != VK_SUCCESS) {
        return Error(result);
    }

    std::vector<DescriptorSet> descSets;
    descSets.reserve(descriptorSets.size());
    std::transform(descriptorSets.cbegin(), descriptorSets.cend(), std::back_inserter(descSets), [&](VkDescriptorSet descriptorSet) { return DescriptorSet(descriptorSet, descriptorPool); });
    return descSets;
}

VkDescriptorSet DescriptorSet::getVkDescriptorSet() const {
    return _descriptorSet;
}

const DescriptorPool& DescriptorSet::getDescriptorPool() const {
    return *_descriptorPool;
}
