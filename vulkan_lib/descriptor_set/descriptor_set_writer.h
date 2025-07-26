#pragma once

#include "vulkan_lib/lib/buffer/buffer.h"
#include "vulkan_lib/memory_objects/buffer.h"
#include "vulkan_lib/memory_objects/texture.h"
#include "vulkan_lib/descriptor_set/descriptor_set.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>

class DescriptorSetWriter {
	uint32_t _binding = 0;
	uint32_t _arrayElement = 0;

	std::vector<VkDescriptorImageInfo> _imageInfos;
	std::vector<VkDescriptorBufferInfo> _bufferInfos;

	std::vector<VkWriteDescriptorSet> _descriptorWrites;

	std::vector<uint32_t> _dynamicBuffersBaseSizes;

public:
	DescriptorSetWriter() = default;

	DescriptorSetWriter& storeTexture(const Texture& texture);

	DescriptorSetWriter& storeBuffer(const Buffer& buffer);

	DescriptorSetWriter& storeDynamicBuffer(const Buffer& buffer, uint32_t dynamicElementSize);

	DescriptorSetWriter& storeBufferArrayElement(const Buffer& buffer);

	void writeDescriptorSet(VkDevice device, const VkDescriptorSet descriptorSet);

	void getDynamicBufferSizesWithOffsets(uint32_t* data, std::initializer_list<uint32_t> offsets) const;
};