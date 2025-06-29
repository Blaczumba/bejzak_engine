#pragma once

#include <cstdint>
#include <numeric>
#include <vector>

#include "memory_objects/buffer.h"
#include "memory_objects/texture/texture.h"
#include "descriptor_set/descriptor_set.h"

enum class TextureHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };
enum class BufferHandle : uint32_t { INVALID = std::numeric_limits<uint32_t>::max() };

class BindlessDescriptorSetWriter {
	std::vector<const Texture*> _textures;
	std::vector<const Buffer*> _buffers;

	const DescriptorSet& _descriptorSet;

public:
	BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet);

	TextureHandle storeTexture(const Texture& texture);

	BufferHandle storeBuffer(const Buffer& buffer);
};