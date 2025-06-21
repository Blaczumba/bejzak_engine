#include "descriptor_binding_manager.h"

#include "descriptor_set/descriptor_pool.h"

#include <vulkan/vulkan.h>


namespace {

constexpr uint32_t UNIFORM_BINDING = 0;
constexpr uint32_t STORAGE_BINDING = 1;
constexpr uint32_t TEXTURE_BINDING = 2;

} //namespace

DescriptorBindingManager::DescriptorBindingManager(const DescriptorSet& descriptorSet) :_descriptorSet(descriptorSet) {

}

TextureHandle DescriptorBindingManager::storeTexture(const Texture& texture) {
	size_t handle = _textures.size();
	_textures.push_back(texture.getVkImageView());

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

BufferHandle DescriptorBindingManager::storeBuffer(const Buffer& buffer) {
	return BufferHandle::INVALID;
}