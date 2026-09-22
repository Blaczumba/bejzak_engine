#pragma once

#include <span>
#include <vector>

#include "common/util/resource_handles.h"
#include "vulkan/resource_manager/ref.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/sampler/sampler.h"

class BindlessDescriptorSetWriter {
  struct TextureResources {
    Ref<Image> imageRef;
    Ref<Sampler> samplerRef;
  };

  struct BufferResources {
    Ref<Buffer> bufferRef;
  };

  BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet) noexcept;

public:
  static std::unique_ptr<BindlessDescriptorSetWriter> create(
      const DescriptorSet& descriptorSet) noexcept;

  UniformTextureHandle writeTexture(
      Ref<Image>& image, Ref<Sampler>& sampler, VkImageView view, VkImageLayout layout);

  void overwriteTexture(
      UniformTextureHandle handle, VkImageView view, VkImageLayout layout, VkSampler sampler);

  // TODO: refactor.
  std::vector<UniformTextureHandle> storeTextures(std::span<const Image> images);

  void removeTexture(UniformTextureHandle handle);

  UniformBufferHandle writeBuffer(
      Ref<Buffer>& bufferRef, VkBufferUsageFlags flags, size_t range, size_t offset = 0);

  // TODO: refactor.
  std::vector<UniformBufferHandle> storeBuffers(std::span<const Buffer> buffers);

  void removeBuffer(UniformBufferHandle handle);

private:
  UniformTextureHandle _nextTextureHandle = UniformTextureHandle(0);
  UniformBufferHandle _nextBufferHandle = UniformBufferHandle(0);

  // TODO: Change to flat_unordered_map.
  std::unordered_map<UniformTextureHandle, TextureResources> _textureDependencies;
  std::vector<UniformTextureHandle> _reclaimedTextureHandles;
  std::unordered_map<UniformBufferHandle, BufferResources> _bufferDependencies;
  std::vector<UniformBufferHandle> _reclaimedBufferHandles;
  const DescriptorSet& _descriptorSet;
};
