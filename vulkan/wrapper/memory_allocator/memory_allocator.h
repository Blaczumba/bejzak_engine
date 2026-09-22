#pragma once

#include <set>
#include <unordered_map>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

class VmaWrapper {
public:
  VmaWrapper(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance);

  VmaWrapper(VmaWrapper&& allocator) noexcept;

  VmaWrapper& operator=(VmaWrapper&& allocator) noexcept;

  ~VmaWrapper();

  struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    void* mappedData;
  };

  struct Image {
    VkImage image;
    VmaAllocation allocation;
  };

  Buffer createVkBuffer(const VkBufferCreateInfo& bufferCreateInfo, VmaMemoryUsage memoryUsage,
                        VmaAllocationCreateFlags flags = 0U);

  void destroyVkBuffer(VkBuffer buffer, const VmaAllocation allocation);

  void sendDataToBufferMemory(
      VkBuffer buffer, const VmaAllocation allocation, const void* data, size_t size);

  Image createVkImage(const VkImageCreateInfo& params, VmaMemoryUsage memoryUsage,
                      VmaAllocationCreateFlags flags = 0U);

  void destroyVkImage(VkImage image, const VmaAllocation allocation);

private:
  VmaAllocator _allocator;
};
