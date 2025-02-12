#include "descriptor_set_layout.h"

#include "logical_device/logical_device.h"

#include <stdexcept>

DescriptorSetLayout::DescriptorSetLayout(const LogicalDevice& logicalDevice) 
	: _logicalDevice(logicalDevice), _binding(0) {

}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_logicalDevice.getVkDevice(), _descriptorSetLayout, nullptr);
}

void DescriptorSetLayout::addLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount, const VkSampler* pImmutableSamplers) {
	_bindings.emplace_back(_binding++, descriptorType, descriptorCount, stageFlags, pImmutableSamplers);
    ++_descriptorTypeOccurances[descriptorType];
}

void DescriptorSetLayout::create() {
    if (_descriptorSetLayout) {
        throw std::runtime_error("error: trying to create already created descriptor set layout");
    }

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(_bindings.size()),
        .pBindings = _bindings.data()
    };

    if (vkCreateDescriptorSetLayout(_logicalDevice.getVkDevice(), &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

const VkDescriptorSetLayout DescriptorSetLayout::getVkDescriptorSetLayout() const {
    return _descriptorSetLayout;
}

const DescriptorTypeCounterDict& DescriptorSetLayout::getDescriptorTypeCounter() const {
    return _descriptorTypeOccurances;
}