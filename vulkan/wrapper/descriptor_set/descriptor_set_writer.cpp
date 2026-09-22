#include "vulkan/wrapper/descriptor_set/descriptor_set_writer.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/descriptor_set/descriptor_set_writer_lib.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"

void DescriptorSetWriter::storeImage(
    VkImageView view, VkImageLayout layout, VkSampler sampler, VkDescriptorType type) {
  _imageInfos.push_back(
      VkDescriptorImageInfo{.sampler = sampler, .imageView = view, .imageLayout = layout});
  _arrayElement = 0;
  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding++,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = type,
    .pImageInfo = &_imageInfos.back()});
}

DescriptorSetWriter& DescriptorSetWriter::storeTexture(
    VkImageView imageView, VkImageLayout layout, VkSampler sampler) {
  storeImage(imageView, layout, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeImageStorage(
    VkImageView imageView, VkImageLayout layout) {
  storeImage(imageView, layout, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeBuffer(
    const Buffer& buffer, VkBufferUsageFlags usage, VkDeviceSize range, VkDeviceSize offset) {
  _bufferInfos.push_back(VkDescriptorBufferInfo{
    .buffer = buffer.getVkBuffer(),
    .offset = offset,
    .range = range,
  });

  _arrayElement = 0;
  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding++,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(usage),
    .pBufferInfo = &_bufferInfos.back()});
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeDynamicBuffer(
    const Buffer& buffer, VkBufferUsageFlags usage, uint32_t dynamicElementSize,
    uint32_t descriptorCount) {
  _arrayElement = 0;
  _bufferInfos.reserve(descriptorCount);
  for (uint32_t i = 0; i < descriptorCount; i++) {
    _bufferInfos.push_back(VkDescriptorBufferInfo{
      .buffer = buffer.getVkBuffer(),
      .offset = i * dynamicElementSize,
      .range = dynamicElementSize});
    _dynamicBuffersBaseSizes.push_back(dynamicElementSize);
    _descriptorWrites.push_back(VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = _binding,
      .dstArrayElement = _arrayElement++,
      .descriptorCount = 1,
      .descriptorType = getDescriptorTypeDynamic(usage),
      .pBufferInfo = &_bufferInfos.back()});
  }
  _binding++;

  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeBufferArrayElement(
    const Buffer& buffer, VkBufferUsageFlags usage, VkDeviceSize range, VkDeviceSize offset) {
  _bufferInfos.push_back(
      VkDescriptorBufferInfo{.buffer = buffer.getVkBuffer(), .offset = offset, .range = range});

  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(usage),
    .pBufferInfo = &_bufferInfos.back()});
  return *this;
}

void DescriptorSetWriter::writeDescriptorSet(VkDevice device, const VkDescriptorSet descriptorSet) {
  for (VkWriteDescriptorSet& descriptorWrite : _descriptorWrites) {
    descriptorWrite.dstSet = descriptorSet;
  }
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(_descriptorWrites.size()),
                         _descriptorWrites.data(), 0, nullptr);
}

void DescriptorSetWriter::getDynamicBufferSizesWithOffsets(
    uint32_t* data, std::initializer_list<uint32_t> offsets) const {
  std::transform(offsets.begin(), offsets.end(), _dynamicBuffersBaseSizes.cbegin(), data,
                 std::multiplies<uint32_t>());
}
