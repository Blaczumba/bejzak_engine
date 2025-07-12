#pragma once

#include "memory_objects/image.h"
#include "status/status.h"

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

	void destroy();

	struct Buffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		void* mappedData;
	};

	struct Image {
		VkImage image;
		VmaAllocation allocation;
	};

	ErrorOr<Buffer> createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0U);
	void destroyVkBuffer(const VkBuffer buffer, const VmaAllocation allocation);
	void sendDataToBufferMemory(const VkBuffer buffer, const VmaAllocation allocation, const void* data, size_t size);
	ErrorOr<Image> createVkImage(const ImageParameters& params, VkImageLayout layout, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0U);
	void destroyVkImage(const VkImage image, const VmaAllocation allocation);

private:
	VmaAllocator _allocator;
};
