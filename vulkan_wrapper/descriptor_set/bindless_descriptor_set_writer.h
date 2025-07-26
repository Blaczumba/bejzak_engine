#pragma once

#include "common/util/bindless_descriptor_handles.h"
#include "vulkan_wrapper/memory_objects/buffer.h"
#include "vulkan_wrapper/memory_objects/texture.h"
#include "vulkan_wrapper/descriptor_set/descriptor_set.h"
#include "vulkan_wrapper/descriptor_set/descriptor_set_writer_lib.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

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