#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "lib/buffer/buffer.h"
#include "logical_device/logical_device.h"
#include "memory_objects/uniform_buffer/uniform_buffer.h"
#include "pipeline/pipeline.h"
#include "status/status.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

DescriptorSet::DescriptorSet(const VkDescriptorSet descriptorSet, const std::shared_ptr<const DescriptorPool>& descriptorPool, const DescriptorSetLayout& layout)
	: _descriptorSet(descriptorSet), _descriptorPool(descriptorPool), _layout(&layout) {}

DescriptorSet::DescriptorSet() : _descriptorSet(VK_NULL_HANDLE) {

}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet) noexcept
    : _descriptorSet(descriptorSet._descriptorSet), _dynamicBuffersBaseSizes(std::move(descriptorSet._dynamicBuffersBaseSizes)), _descriptorPool(std::move(descriptorSet._descriptorPool)), _layout(descriptorSet._layout) {

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

ErrorOr<DescriptorSet> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool, const DescriptorSetLayout& layout) {
    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout.getVkDescriptorSetLayout(),
    };

    VkDescriptorSet descriptorSet;
    if (VkResult result = vkAllocateDescriptorSets(descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, &descriptorSet); result != VK_SUCCESS) {
        return Error(result);
    }
    return DescriptorSet(descriptorSet, descriptorPool, layout);
}

ErrorOr<std::vector<DescriptorSet>> DescriptorSet::create(const std::shared_ptr<const DescriptorPool>& descriptorPool, const DescriptorSetLayout& layout, uint32_t numSets) {
    const std::vector<VkDescriptorSetLayout> layouts(numSets, layout.getVkDescriptorSetLayout());
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
    std::transform(descriptorSets.cbegin(), descriptorSets.cend(), std::back_inserter(descSets), [&](const VkDescriptorSet descriptorSet) { return DescriptorSet(descriptorSet, descriptorPool, layout); });
    return descSets;
}

void DescriptorSet::writeDescriptorSet(std::initializer_list<UniformBuffer*> uniformBuffers) {
    writeDescriptorSetImpl(uniformBuffers);
}

void DescriptorSet::writeDescriptorSet(std::span<const UniformBuffer*> uniformBuffers) {
    writeDescriptorSetImpl(uniformBuffers);
}

void DescriptorSet::writeDescriptorSetImpl(std::span<const UniformBuffer* const> uniformBuffers) {
    lib::Buffer<VkWriteDescriptorSet> descriptorWrites(uniformBuffers.size());
    uint32_t bindingIndex = 0;
    for (const UniformBuffer* uniformBuffer : uniformBuffers) {
        descriptorWrites[bindingIndex] = uniformBuffer->getVkWriteDescriptorSet(_descriptorSet, bindingIndex);
        if (uniformBuffer->getVkDescriptorType() == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            _dynamicBuffersBaseSizes.emplace_back(uniformBuffer->getSize());
        }
        ++bindingIndex;
    }
    vkUpdateDescriptorSets(_descriptorPool->getLogicalDevice().getVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorSet::bind(const VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides) {
    std::array<uint32_t, 4> sizes;
    std::transform(dynamicOffsetStrides.begin(), dynamicOffsetStrides.end(), _dynamicBuffersBaseSizes.cbegin(), sizes.begin(), std::multiplies<uint32_t>());
    vkCmdBindDescriptorSets(commandBuffer, pipeline.getVkPipelineBindPoint(), pipeline.getVkPipelineLayout(), 0, 1, &_descriptorSet, dynamicOffsetStrides.size(), sizes.data());
}

const VkDescriptorSet DescriptorSet::getVkDescriptorSet() const {
    return _descriptorSet;
}

const DescriptorPool& DescriptorSet::getDescriptorPool() const {
    return *_descriptorPool;
}
