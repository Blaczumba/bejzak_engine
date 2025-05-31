#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "lib/buffer/buffer.h"
#include "lib/status/status.h"
#include "logical_device/logical_device.h"
#include "memory_objects/uniform_buffer/uniform_buffer.h"
#include "pipeline/pipeline.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

DescriptorSet::DescriptorSet(const VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool)
	: _descriptorSet(descriptorSet), _descriptorPool(descriptorPool) {}

DescriptorSet::DescriptorSet() : _descriptorSet(VK_NULL_HANDLE) {

}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet) noexcept
    : _descriptorSet(descriptorSet._descriptorSet), _dynamicBuffersBaseSizes(std::move(descriptorSet._dynamicBuffersBaseSizes)), _descriptorPool(std::move(descriptorSet._descriptorPool)) {

}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& descriptorSet) noexcept {
    if (this == &descriptorSet) {
        return *this;
    }
    _descriptorSet = descriptorSet._descriptorSet;
    _dynamicBuffersBaseSizes = std::move(descriptorSet._dynamicBuffersBaseSizes);
    _descriptorPool = std::move(descriptorSet._descriptorPool);

    return *this;
}

lib::ErrorOr<DescriptorSet> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool) {
    const VkDescriptorSetLayout layout = descriptorPool->getDescriptorSetLayout().getVkDescriptorSetLayout();

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        return lib::Error("failed to allocate descriptor sets!");
    }
    return DescriptorSet(descriptorSet, descriptorPool);
}

lib::ErrorOr<std::vector<DescriptorSet>> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool, uint32_t numSets) {
    const std::vector<VkDescriptorSetLayout> layouts(numSets, descriptorPool->getDescriptorSetLayout().getVkDescriptorSetLayout());
    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = numSets,
        .pSetLayouts = layouts.data(),
    };

    lib::Buffer<VkDescriptorSet> descriptorSets(numSets);
    if (vkAllocateDescriptorSets(descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        return lib::Error("failed to allocate descriptor sets!");
    }

    std::vector<DescriptorSet> descSets;
    descSets.reserve(descriptorSets.size());
    std::transform(descriptorSets.cbegin(), descriptorSets.cend(), std::back_inserter(descSets), [&](const VkDescriptorSet descriptorSet) { return DescriptorSet(descriptorSet, descriptorPool); });
    return descSets;
}

void DescriptorSet::writeDescriptorSet(std::initializer_list<UniformBuffer*> uniformBuffers) {
    lib::Buffer<VkWriteDescriptorSet> descriptorWrites(uniformBuffers.size());
    for (size_t i = 0; i < uniformBuffers.size(); ++i) {
        const UniformBuffer* uniformBuffer = *(uniformBuffers.begin() + i);
        descriptorWrites[i] = uniformBuffer->getVkWriteDescriptorSet(_descriptorSet, i);
        if (uniformBuffer->getVkDescriptorType() == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            _dynamicBuffersBaseSizes.emplace_back(uniformBuffer->getSize());
        }
    }
    vkUpdateDescriptorSets(_descriptorPool->getLogicalDevice().getVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorSet::bind(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides) {
    std::array<uint32_t, 4> sizes;
    std::transform(dynamicOffsetStrides.begin(), dynamicOffsetStrides.end(), _dynamicBuffersBaseSizes.cbegin(), sizes.begin(), std::multiplies<uint32_t>());
    vkCmdBindDescriptorSets(commandBuffer, pipeline.getVkPipelineBindPoint(), pipeline.getVkPipelineLayout(), 0, 1, &_descriptorSet, dynamicOffsetStrides.size(), sizes.data());
}

const VkDescriptorSet DescriptorSet::getVkDescriptorSet(size_t i) const {
    return _descriptorSet;
}
