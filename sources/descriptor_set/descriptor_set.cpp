#include "descriptor_set.h"

#include "descriptor_pool.h"
#include "descriptor_set_layout.h"
#include "logical_device/logical_device.h"
#include "memory_objects/uniform_buffer/uniform_buffer.h"
#include "pipeline/pipeline.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

DescriptorSet::DescriptorSet(const std::shared_ptr<const DescriptorPool>& descriptorPool)
	: _descriptorPool(descriptorPool) {
    const VkDescriptorSetLayout layout = _descriptorPool->getDescriptorSetLayout().getVkDescriptorSetLayout();

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _descriptorPool->getVkDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    if (vkAllocateDescriptorSets(_descriptorPool->getLogicalDevice().getVkDevice(), &allocInfo, &_descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void DescriptorSet::updateDescriptorSet(const std::vector<UniformBuffer*>& uniformBuffers) {

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(uniformBuffers.size());

    for (size_t j = 0; j < uniformBuffers.size(); j++) {
        const UniformBuffer* uniformBuffer = uniformBuffers[j];

        descriptorWrites.emplace_back(uniformBuffer->getVkWriteDescriptorSet(_descriptorSet, j));

        if (uniformBuffer->getVkDescriptorType() == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            _dynamicBuffersBaseSizes.emplace_back(uniformBuffer->getSize());
        }
    }

    vkUpdateDescriptorSets(_descriptorPool->getLogicalDevice().getVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorSet::bind(VkCommandBuffer commandBuffer, const Pipeline& pipeline, std::initializer_list<uint32_t> dynamicOffsetStrides) {
    std::array<uint32_t, 16> sizes;
    std::transform(dynamicOffsetStrides.begin(), dynamicOffsetStrides.end(), _dynamicBuffersBaseSizes.cbegin(), sizes.begin(), std::multiplies<uint32_t>());

    vkCmdBindDescriptorSets(commandBuffer, pipeline.getVkPipelineBindPoint(), pipeline.getVkPipelineLayout(), 0, 1, &_descriptorSet, dynamicOffsetStrides.size(), sizes.data());
}

const VkDescriptorSet DescriptorSet::getVkDescriptorSet(size_t i) const {
    return _descriptorSet;
}
