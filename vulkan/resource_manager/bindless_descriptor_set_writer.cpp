#include "bindless_descriptor_set_writer.h"

#include <format>
#include <ranges>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/util/engine_exception.h"
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
    Ref<Image>& imageRef, Ref<Sampler>& samplerRef, VkImageView view, VkImageLayout layout,
    VkSampler sampler) {
  const UniformTextureHandle handle = getNextHandle(_texturesMap.size(), _missingTextures);
  if (!_texturesMap.insert(*handle, TextureResources{imageRef, samplerRef})) [[unlikely]] {
    throw EngineException(std::format(
        "BindlessDescriptorSetWriter::storeTexture: Failed to insert Texture Handle = {}.",
        *handle));
  }

  overwriteTexture(handle, view, layout, sampler);
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

std::vector<UniformTextureHandle> BindlessDescriptorSetWriter::storeTextures(
    std::span<const Image> textures) {
  std::vector<UniformTextureHandle> handles;
  // handles.reserve(textures.size());
  // for (uint32_t i = 0; i < textures.size(); i++) {
  //   const UniformTextureHandle handle = getNextHandle(_texturesMap.size(), _missingTextures);
  //   if (!_texturesMap.insert(*handle)) [[unlikely]] {
  //     throw EngineException(std::format(
  //         "BindlessDescriptorSetWriter::storeTextures: Failed to insert Texture Handle = {}.",
  //         *handle));
  //   }

  //  handles.push_back(handle);
  //}

  // lib::Buffer<VkDescriptorImageInfo> imageInfos(textures.size());
  // lib::Buffer<VkWriteDescriptorSet> writes(textures.size());

  // for (auto&& [imageInfo, write, handle, texture] :
  //      std::views::zip(imageInfos, writes, handles, textures)) {
  //   imageInfo = VkDescriptorImageInfo{
  //     // TODO: Use samplers.
  //     .sampler = VK_NULL_HANDLE,
  //     .imageView = texture.getVkImageView(),
  //     .imageLayout = texture.getVkImageLayout()};

  //  write = VkWriteDescriptorSet{
  //    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //    .dstSet = _descriptorSet.getVkDescriptorSet(),
  //    .dstBinding = TEXTURE_BINDING,
  //    .dstArrayElement = static_cast<uint32_t>(*handle),
  //    .descriptorCount = 1,
  //    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
  //    .pImageInfo = &imageInfo};
  //}

  // vkUpdateDescriptorSets(_descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(),
  //                        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
  return handles;
}

void BindlessDescriptorSetWriter::removeTexture(UniformTextureHandle handle) {
  _missingTextures.push_back(handle);
  _texturesMap.erase(*handle);
}

UniformBufferHandle BindlessDescriptorSetWriter::writeBuffer(
    Ref<Buffer>& bufferRef, VkBuffer buffer, const BufferMetadata& metadata,
    std::optional<size_t> size, size_t offset) {
  const UniformBufferHandle handle = getNextHandle(_buffersMap.size(), _missingBuffers);
  if (!_buffersMap.insert(*handle, BufferResources{bufferRef})) [[unlikely]] {
    throw EngineException(std::format(
        "BindlessDescriptorSetWriter::storeBuffer: Failed to insert Buffer Handle = {}.", *handle));
  }

  size_t range = size.value_or(metadata.size);
  if (range + offset > metadata.size) [[unlikely]] {
    throw EngineException(
        std::format(
            "BindlessDescriptorSetWriter::storeBuffer: Buffer range " "(offset = {}, size " "= " "{" "}" ")" " " "e" "x" "c" "e" "e" "d" "s" " " "buffer size " "({}).",
            offset, range, metadata.size));
  }

  const VkDescriptorBufferInfo bufferInfo = {.buffer = buffer, .offset = offset, .range = range};
  const VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet = _descriptorSet.getVkDescriptorSet(),
    .dstBinding = UNIFORM_BINDING,
    .dstArrayElement = static_cast<uint32_t>(*handle),
    .descriptorCount = 1,
    .descriptorType = getDescriptorType(metadata.usage),
    .pBufferInfo = &bufferInfo};

  vkUpdateDescriptorSets(
      _descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

  return handle;
}

std::vector<UniformBufferHandle> BindlessDescriptorSetWriter::storeBuffers(
    std::span<const Buffer> buffers) {
  std::vector<UniformBufferHandle> handles;
  // handles.reserve(buffers.size());
  // for (uint32_t i = 0; i < buffers.size(); i++) {
  //   const UniformBufferHandle handle = getNextHandle(_buffersMap.size(), _missingBuffers);
  //   if (!_buffersMap.insert(*handle)) [[unlikely]] {
  //     throw EngineException(std::format(
  //         "BindlessDescriptorSetWriter::storeBuffers: Failed to insert Buffer Handle = {}.",
  //         *handle));
  //   }

  //  handles.push_back(handle);
  //}

  // lib::Buffer<VkDescriptorBufferInfo> bufferInfos(buffers.size());
  // lib::Buffer<VkWriteDescriptorSet> writes(buffers.size());

  // for (auto&& [bufferInfo, write, handle, buffer] :
  //      std::views::zip(bufferInfos, writes, handles, buffers)) {
  //   bufferInfo = VkDescriptorBufferInfo{.buffer = buffer.getVkBuffer(), .range =
  //   buffer.getSize()};

  //  write = VkWriteDescriptorSet{
  //    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  //    .dstSet = _descriptorSet.getVkDescriptorSet(),
  //    .dstBinding = UNIFORM_BINDING,
  //    .dstArrayElement = static_cast<uint32_t>(*handle),
  //    .descriptorCount = 1,
  //    .descriptorType = getDescriptorType(buffer.getUsage()),
  //    .pBufferInfo = &bufferInfo};
  //}

  // vkUpdateDescriptorSets(_descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(),
  //                        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
  return handles;
}

void BindlessDescriptorSetWriter::removeBuffer(UniformBufferHandle handle) {
  _missingBuffers.push_back(handle);
  _buffersMap.erase(*handle);
}
