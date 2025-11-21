#include "vulkan/wrapper/descriptor_set/bindless_descriptor_set_writer.h"

#include <vulkan/vulkan.h>

#include "vulkan/wrapper/descriptor_set/descriptor_pool.h"

namespace {

constexpr uint32_t UNIFORM_BINDING = 0;
constexpr uint32_t TEXTURE_BINDING = 1;
constexpr uint32_t STORAGE_BINDING = 2;

uint32_t getNextHandle(uint32_t elementsCount, std::vector<uint32_t>& missingBindings) {
  if (missingBindings.empty()) {
    return elementsCount;
  }
  uint32_t it = missingBindings.back();
  missingBindings.pop_back();
  return it;
}

}  // namespace

BindlessDescriptorSetWriter::BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet)
  : _descriptorSet(descriptorSet) {}

TextureHandle BindlessDescriptorSetWriter::storeTexture(const Texture& texture) {
  const uint32_t handle = getNextHandle(_texturesMap.size(), _missingTextures);
  _texturesMap.emplace(handle, &texture);

  const VkDescriptorImageInfo imageInfo = {
    .sampler = texture.getVkSampler(),
    .imageView = texture.getVkImageView(),
    .imageLayout = texture.getVkImageLayout()};

  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = _descriptorSet.getVkDescriptorSet(),
    .dstBinding = TEXTURE_BINDING,
    .dstArrayElement = handle,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &imageInfo};

  vkUpdateDescriptorSets(
      _descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

  return static_cast<TextureHandle>(handle);
}

void BindlessDescriptorSetWriter::removeTexture(TextureHandle handle) {
  _missingTextures.push_back(static_cast<uint32_t>(handle));
  _texturesMap.erase(static_cast<uint32_t>(handle));
}

BufferHandle BindlessDescriptorSetWriter::storeBuffer(const Buffer& buffer) {
  const size_t handle = getNextHandle(_buffersMap.size(), _missingBuffers);
  _buffersMap.emplace(handle, &buffer);

  const VkDescriptorBufferInfo bufferInfo = {
    .buffer = buffer.getVkBuffer(), .range = buffer.getSize()};

  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = _descriptorSet.getVkDescriptorSet(),
    .dstBinding = UNIFORM_BINDING,
    .dstArrayElement = static_cast<uint32_t>(handle),
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(buffer.getUsage()),
    .pBufferInfo = &bufferInfo};

  vkUpdateDescriptorSets(
      _descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

  return static_cast<BufferHandle>(handle);
}

void BindlessDescriptorSetWriter::removeBuffer(BufferHandle handle) {
  _missingBuffers.push_back(static_cast<uint32_t>(handle));
  _buffersMap.erase(static_cast<uint32_t>(handle));
}
