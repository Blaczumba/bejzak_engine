#include "descriptor_set/descriptor_set_writer.h"

#include "descriptor_set/descriptor_set_writer_lib.h"

DescriptorSetWriter& DescriptorSetWriter::storeTexture(const Texture& texture) {
	_imageInfos.emplace_back(
		VkDescriptorImageInfo {
			.sampler = texture.getVkSampler(),
			.imageView = texture.getVkImageView(),
			.imageLayout = texture.getImageParameters().layout
		}
	);

	_descriptorWrites.emplace_back(
		VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstBinding = binding++,
			.dstArrayElement = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &_imageInfos.back()
		}
	);
	return *this;
}

DescriptorSetWriter& DescriptorSetWriter::storeBuffer(const Buffer& buffer, bool isDynamic) {
	_bufferInfos.push_back(
		VkDescriptorBufferInfo {
			.buffer = buffer.getVkBuffer(),
			.range = buffer.getSize()
		}
	);

	const VkDescriptorType type = getDescriptorType(buffer.getUsage(), isDynamic);
	_descriptorWrites.push_back(
		VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstBinding = binding++,
			.dstArrayElement = 1,
			.descriptorCount = 1,
			.descriptorType = type,
			.pBufferInfo = &_bufferInfos.back()
		}
	);
	return *this;
}

void DescriptorSetWriter::writeDescriptorSet(const VkDevice device, const VkDescriptorSet descriptorSet) {
	for (auto& descriptorWrite : _descriptorWrites) {
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(_descriptorWrites.size()), _descriptorWrites.data(), 0, nullptr);
}

void DescriptorSetWriter::getDynamicBufferSizesWithOffsets(uint32_t* data, std::initializer_list<uint32_t> offsets) const {
	std::transform(offsets.begin(), offsets.end(), _dynamicBuffersBaseSizes.cbegin(), data, std::multiplies<uint32_t>());
}