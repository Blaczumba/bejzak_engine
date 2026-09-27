#include "bindless_descriptor_set_writer.h"

#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan/resource_manager/util.h"
#include "vulkan/wrapper/descriptor_set/descriptor_pool.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set_writer_lib.h"

namespace {

constexpr uint32_t UNIFORM_BINDING = 0;
constexpr uint32_t TEXTURE_BINDING = 1;
constexpr uint32_t STORAGE_BINDING = 2;

}  // namespace

BindlessDescriptorSetWriter::BindlessDescriptorSetWriter(
    const DescriptorSet& descriptorSet) noexcept
  : _descriptorSet(descriptorSet) {}

std::unique_ptr<BindlessDescriptorSetWriter> BindlessDescriptorSetWriter::create(
    const DescriptorSet& descriptorSet) noexcept {
  return std::unique_ptr<BindlessDescriptorSetWriter>(
      new BindlessDescriptorSetWriter(descriptorSet));
}

UniformTextureHandle BindlessDescriptorSetWriter::writeTexture(
    Ref<Image>& image, Ref<Sampler>& sampler, VkImageView view, VkImageLayout layout) {
  const UniformTextureHandle handle =
      getNextHandleFromReclaimed(_nextTextureHandle, _reclaimedTextureHandles);
  _textureDependencies.emplace(*handle, TextureResources{image, sampler});
  overwriteTexture(handle, view, layout, sampler.getUnderlyingResource());
  return handle;
}

void BindlessDescriptorSetWriter::overwriteTexture(
    UniformTextureHandle handle, VkImageView view, VkImageLayout layout, VkSampler sampler) {
  const VkDescriptorImageInfo imageInfo = {
    .sampler = sampler, .imageView = view, .imageLayout = layout};

  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = _descriptorSet.getVkDescriptorSet(),
    .dstBinding = TEXTURE_BINDING,
    .dstArrayElement = static_cast<uint32_t>(*handle),
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo = &imageInfo};

  vkUpdateDescriptorSets(
      _descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);
}

void BindlessDescriptorSetWriter::removeTexture(UniformTextureHandle handle) {
  _reclaimedTextureHandles.push_back(handle);
  _textureDependencies.erase(handle);
}

UniformBufferHandle BindlessDescriptorSetWriter::writeBuffer(
    const Ref<Buffer>& bufferRef, VkBufferUsageFlags usage, size_t range, size_t offset) {
  const UniformBufferHandle handle =
      getNextHandleFromReclaimed(_nextBufferHandle, _reclaimedBufferHandles);
  _bufferDependencies.emplace(*handle, BufferResources{bufferRef});

  const VkDescriptorBufferInfo bufferInfo = {
    .buffer = bufferRef.getUnderlyingResource(), .offset = offset, .range = range};
  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = _descriptorSet.getVkDescriptorSet(),
    .dstBinding = UNIFORM_BINDING,
    .dstArrayElement = static_cast<uint32_t>(*handle),
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(usage),
    .pBufferInfo = &bufferInfo};

  vkUpdateDescriptorSets(
      _descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

  return handle;
}

void BindlessDescriptorSetWriter::removeBuffer(UniformBufferHandle handle) {
  _reclaimedBufferHandles.push_back(handle);
  _bufferDependencies.erase(handle);
}
