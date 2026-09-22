#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/sampler/sampler.h"

class DescriptorSetWriter {
public:
  DescriptorSetWriter() noexcept = default;

  DescriptorSetWriter& storeTexture(VkImageView imageView, VkImageLayout layout, VkSampler sampler);

  DescriptorSetWriter& storeImageStorage(VkImageView imageView, VkImageLayout layout);

  DescriptorSetWriter& storeBuffer(
      const Buffer& buffer, VkBufferUsageFlags usage, VkDeviceSize range, VkDeviceSize offset = 0);

  DescriptorSetWriter& storeDynamicBuffer(
      const Buffer& buffer, VkBufferUsageFlags usage, uint32_t dynamicElementSize,
      uint32_t descriptorCount = 1);

  DescriptorSetWriter& storeBufferArrayElement(
      const Buffer& buffer, VkBufferUsageFlags usage, VkDeviceSize range, VkDeviceSize offset);

  void writeDescriptorSet(VkDevice device, const VkDescriptorSet descriptorSet);

  void getDynamicBufferSizesWithOffsets(
      uint32_t* data, std::initializer_list<uint32_t> offsets) const;

private:
  void storeImage(VkImageView view, VkImageLayout layout, VkSampler sampler, VkDescriptorType type);

  uint32_t _binding = 0;
  uint32_t _arrayElement = 0;

  std::vector<VkDescriptorImageInfo> _imageInfos;
  std::vector<VkDescriptorBufferInfo> _bufferInfos;

  std::vector<VkWriteDescriptorSet> _descriptorWrites;

  std::vector<uint32_t> _dynamicBuffersBaseSizes;
};
