#include "bindless_descriptor_set_writer.h"

#include "descriptor_set/descriptor_pool.h"

#include <vulkan/vulkan.h>


namespace {

constexpr uint32_t UNIFORM_BINDING = 0;
constexpr uint32_t STORAGE_BINDING = 1;
constexpr uint32_t TEXTURE_BINDING = 2;

constexpr VkDescriptorType getDescriptorType(VkBufferUsageFlags usageFlags) {
	switch (usageFlags) {
	case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	default:
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

} //namespace

BindlessDescriptorSetWriter::BindlessDescriptorSetWriter(const DescriptorSet& descriptorSet) :_descriptorSet(descriptorSet) {

}

TextureHandle BindlessDescriptorSetWriter::storeTexture(const Texture& texture) {
	const size_t handle = _textures.size();
	_textures.push_back(&texture);

	const VkDescriptorImageInfo imageInfo = {
		.sampler = texture.getVkSampler(),
		.imageView = texture.getVkImageView(),
		.imageLayout = texture.getImageParameters().layout
	};

	const VkWriteDescriptorSet write = {
		.dstSet = _descriptorSet.getVkDescriptorSet(),
		.dstBinding = TEXTURE_BINDING,
		.dstArrayElement = static_cast<uint32_t>(handle),
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo
	};

	vkUpdateDescriptorSets(_descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

	return static_cast<TextureHandle>(handle);
}

BufferHandle BindlessDescriptorSetWriter::storeBuffer(const Buffer& buffer) {
	const size_t handle = _buffers.size();
	_buffers.push_back(&buffer);

	const VkDescriptorBufferInfo bufferInfo = {
		.buffer = buffer.getVkBuffer(),
		.range = buffer.getSize()
	};

	const VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = _descriptorSet.getVkDescriptorSet(),
		.dstBinding = UNIFORM_BINDING,
		.dstArrayElement = static_cast<uint32_t>(handle),
		.descriptorCount = 1,
		.descriptorType = getDescriptorType(buffer.getUsage()),
		.pBufferInfo = &bufferInfo
	};

	vkUpdateDescriptorSets(_descriptorSet.getDescriptorPool().getLogicalDevice().getVkDevice(), 1, &write, 0, nullptr);

	return static_cast<BufferHandle>(handle);
}