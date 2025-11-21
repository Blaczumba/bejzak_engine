#include "vulkan/wrapper/descriptor_set/descriptor_set_writer.h"

#include "vulkan/wrapper/descriptor_set/descriptor_set_writer_lib.h"

DescriptorSetWriter& DescriptorSetWriter::storeTexture(const Texture& texture) {
  _imageInfos.push_back(VkDescriptorImageInfo{
    .sampler = texture.getVkSampler(),
    .imageView = texture.getVkImageView(),
    .imageLayout = texture.getVkImageLayout()});
  _arrayElement = 0;
  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding++,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &_imageInfos.back()});
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeBuffer(const Buffer& buffer) {
  _bufferInfos.push_back(
      VkDescriptorBufferInfo{.buffer = buffer.getVkBuffer(), .range = buffer.getSize()});

  _arrayElement = 0;
  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding++,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(buffer.getUsage()),
    .pBufferInfo = &_bufferInfos.back()});
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeDynamicBuffer(
    const Buffer& buffer, uint32_t dynamicElementSize) {
  _bufferInfos.push_back(
      VkDescriptorBufferInfo{.buffer = buffer.getVkBuffer(), .range = dynamicElementSize});

  _dynamicBuffersBaseSizes.push_back(dynamicElementSize);
  _arrayElement = 0;
  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding++,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = getDescriptorTypeDynamic(buffer.getUsage()),
    .pBufferInfo = &_bufferInfos.back()});
  return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeBufferArrayElement(const Buffer& buffer) {
  _bufferInfos.push_back(
      VkDescriptorBufferInfo{.buffer = buffer.getVkBuffer(), .range = buffer.getSize()});

  _descriptorWrites.push_back(VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = _binding,
    .dstArrayElement = _arrayElement++,
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(buffer.getUsage()),
    .pBufferInfo = &_bufferInfos.back()});
  return *this;
}

void DescriptorSetWriter::writeDescriptorSet(VkDevice device, const VkDescriptorSet descriptorSet) {
  for (auto& descriptorWrite : _descriptorWrites) {
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
