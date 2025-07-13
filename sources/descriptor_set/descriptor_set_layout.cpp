#include "descriptor_set_layout.h"

#include "logical_device/logical_device.h"

DescriptorSetLayout::DescriptorSetLayout(const LogicalDevice& logicalDevice, VkDescriptorSetLayout layout) : _logicalDevice(&logicalDevice), _descriptorSetLayout(layout) {

}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& layout) noexcept
    : _descriptorSetLayout(std::exchange(layout._descriptorSetLayout, VK_NULL_HANDLE)), _logicalDevice(layout._logicalDevice) {

}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& layout) noexcept {
    if (&layout == this) {
        return *this;
    }
    _descriptorSetLayout = std::exchange(layout._descriptorSetLayout, VK_NULL_HANDLE);
    _logicalDevice = std::exchange(layout._logicalDevice, nullptr);
    return *this;
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_logicalDevice->getVkDevice(), _descriptorSetLayout, nullptr);
}

ErrorOr<DescriptorSetLayout> DescriptorSetLayout::create(const LogicalDevice& logicalDevice, std::span<const VkDescriptorSetLayoutBinding> bindings, std::span<const VkDescriptorBindingFlags> bindFlags, VkDescriptorSetLayoutCreateFlags flags) {
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindFlags.size()),
    };

    if (!bindFlags.empty()) {
        bindingFlags.pBindingFlags = bindFlags.data();
    }

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlags,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout descriptorSetLayout;
    if (VkResult result = vkCreateDescriptorSetLayout(logicalDevice.getVkDevice(), &layoutInfo, nullptr, &descriptorSetLayout); result != VK_SUCCESS) {
        return Error(result);
    }
    return DescriptorSetLayout(logicalDevice, descriptorSetLayout);
}

VkDescriptorSetLayout DescriptorSetLayout::getVkDescriptorSetLayout() const {
    return _descriptorSetLayout;
}
