#pragma once

#include <cstdint>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/util/bindless_descriptor_handles.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set_writer_lib.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/texture.h"

class BindlessDescriptorSetWriter {
  const DescriptorSet& _descriptorSet;

  std::unordered_map<uint32_t, const Texture*> _texturesMap;  // TODO: Change to flat_unordered_map
  std::vector<uint32_t> _missingTextures;
  std::unordered_map<uint32_t, const Buffer*> _buffersMap;  // TODO: Change to flat_unordered_map
  std::vector<uint32_t> _missingBuffers;

public:
  BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet);

  TextureHandle storeTexture(const Texture& texture);

  void removeTexture(TextureHandle handle);

  BufferHandle storeBuffer(const Buffer& buffer);

  void removeBuffer(BufferHandle handle);
};
