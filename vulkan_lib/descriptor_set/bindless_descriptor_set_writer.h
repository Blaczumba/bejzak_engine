#pragma once

#include "vulkan_lib/memory_objects/buffer.h"
#include "vulkan_lib/memory_objects/texture.h"
#include "vulkan_lib/descriptor_set/descriptor_set.h"
#include "vulkan_lib/descriptor_set/descriptor_set_writer_lib.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

enum class TextureHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };
enum class BufferHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };

class BindlessDescriptorSetWriter {
	const DescriptorSet& _descriptorSet;
	
	std::unordered_map<uint32_t, const Texture*> _texturesMap; // TODO: Change to flat_unordered_map
	std::vector<uint32_t> _missingTextures;
	std::unordered_map<uint32_t, const Buffer*> _buffersMap; // TODO: Change to flat_unordered_map
	std::vector<uint32_t> _missingBuffers;

public:
	BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet);

	TextureHandle storeTexture(const Texture& texture);

	void removeTexture(TextureHandle handle);

	BufferHandle storeBuffer(const Buffer& buffer);

	void removeBuffer(BufferHandle handle);
};