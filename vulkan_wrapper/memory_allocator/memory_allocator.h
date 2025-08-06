#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/memory_objects/image.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <unordered_map>
#include <set>

class VmaWrapper {
public:
	VmaWrapper(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance);

	VmaWrapper(VmaWrapper&& allocator) noexcept;

	VmaWrapper& operator=(VmaWrapper&& allocator) noexcept;

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

	void destroyVkBuffer(VkBuffer buffer, const VmaAllocation allocation);

	void sendDataToBufferMemory(VkBuffer buffer, const VmaAllocation allocation, const void* data, size_t size);

	ErrorOr<Image> createVkImage(const ImageParameters& params, VkImageLayout layout, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0U);

	void destroyVkImage(VkImage image, const VmaAllocation allocation);

private:
	VmaAllocator _allocator;
};
