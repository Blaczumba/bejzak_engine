#include "descriptor_set_layout.h"

#include "logical_device/logical_device.h"

#include <stdexcept>

DescriptorSetLayout::DescriptorSetLayout(const LogicalDevice& logicalDevice) 
	: _logicalDevice(logicalDevice), _binding(0) {
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_logicalDevice.getVkDevice(), _descriptorSetLayout, nullptr);
}

void DescriptorSetLayout::addLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount, VkDescriptorBindingFlags bindingFlags, const VkSampler* pImmutableSamplers) {
	_bindings.emplace_back(_binding++, descriptorType, descriptorCount, stageFlags, pImmutableSamplers);
    _bindingFlags.push_back(bindingFlags);
    ++_descriptorTypeOccurances[descriptorType];
}

std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::create(const LogicalDevice& logicalDevice) {
    return std::unique_ptr<DescriptorSetLayout>(new DescriptorSetLayout(logicalDevice));
}

Status DescriptorSetLayout::build(VkDescriptorSetLayoutCreateFlags flags) {
    if (_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_logicalDevice.getVkDevice(), _descriptorSetLayout, nullptr);
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(_bindingFlags.size()),
    };
    bindingFlags.pBindingFlags = _bindingFlags.empty() ? nullptr : _bindingFlags.data();

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlags,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(_bindings.size()),
        .pBindings = _bindings.data()
    };

    if (VkResult result = vkCreateDescriptorSetLayout(_logicalDevice.getVkDevice(), &layoutInfo, nullptr, &_descriptorSetLayout); result != VK_SUCCESS) {
        return Error(result);
    }
    return StatusOk();
}

const VkDescriptorSetLayout& DescriptorSetLayout::getVkDescriptorSetLayout() const {
    return _descriptorSetLayout;
}

const DescriptorTypeCounterDict& DescriptorSetLayout::getDescriptorTypeCounter() const {
    return _descriptorTypeOccurances;
}
