#pragma once

#include <vulkan/vulkan.h>

constexpr VkDescriptorType getDescriptorType(VkBufferUsageFlags usageFlags, bool isDynamic = false) {
	switch (isDynamic) {
	case false:
		if ((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if ((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case true:
		if ((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		if ((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}