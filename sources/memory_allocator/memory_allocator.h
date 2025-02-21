#pragma once

#include <memory_objects/texture/image.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <unordered_map>
#include <set>

class VmaWrapper {
public:
	VmaWrapper(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkInstance instance);
	VmaWrapper(const VmaWrapper& allocator) = delete;
	VmaWrapper(VmaWrapper&& allocator);
	~VmaWrapper();

	void destroy() {
		vmaDestroyAllocator(_allocator);
		_allocator = nullptr;
	}

	std::tuple<VkBuffer, VmaAllocation, void*> createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0U);
	void destroyVkBuffer(const VkBuffer buffer, const VmaAllocation allocation);
	void sendDataToBufferMemory(const VkBuffer buffer, const VmaAllocation allocation, const void* data, size_t size);
	std::pair<VkImage, VmaAllocation> createVkImage(const ImageParameters& params, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0U);
	void destroyVkImage(const VkImage image, const VmaAllocation allocation);

private:
	VmaAllocator _allocator;
};
