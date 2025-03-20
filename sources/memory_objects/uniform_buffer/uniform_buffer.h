#pragma once

#include <logical_device/logical_device.h>
#include <memory_objects/texture/texture.h>
#include <memory_objects/buffer_deallocator.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace {

VkDeviceSize getMemoryAlignment(size_t size, size_t minUboAlignment) {
	return minUboAlignment > 0 ? (size + minUboAlignment - 1) & ~(minUboAlignment - 1) : size;
}

}


class UniformBuffer {
protected:
	const VkDescriptorType _type;
	uint32_t _size;

public:
	UniformBuffer(VkDescriptorType type) : _type(type) {}
	virtual ~UniformBuffer() = default;

	virtual VkWriteDescriptorSet getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const =0;

	VkDescriptorType getVkDescriptorType() const { return _type; }
	uint32_t getSize() const { return _size; }

};

class UniformBufferTexture : public UniformBuffer {
	const Texture& _texture;
	const VkDescriptorImageInfo _imageInfo;

public:
	UniformBufferTexture(const Texture& texture) 
		: UniformBuffer(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER), _texture(texture), 
		_imageInfo{ texture.getVkSampler(), texture.getVkImageView(), texture.getImageParameters().layout } {

	}

	~UniformBufferTexture() override = default;
	const Texture* getTexturePtr() const { return &_texture; }

	VkWriteDescriptorSet getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const override {
		return VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = _type,
			.pImageInfo = &_imageInfo
		};
	}
};

template<typename UniformBufferType>
class UniformBufferData : public UniformBuffer {
	VkBuffer _uniformBuffer;
	Allocation _allocation;
	void* _uniformBufferMapped;

	VkDescriptorBufferInfo _bufferInfo;
	const uint32_t _count;

	const LogicalDevice& _logicalDevice;

public:
	UniformBufferData(const LogicalDevice& logicalDevice, uint32_t count = 1);
	~UniformBufferData() override;

	VkWriteDescriptorSet getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const override;
	void updateUniformBuffer(const UniformBufferType& object, size_t index = 0);

private:
	struct Allocator {
		Allocation& allocation;
		const size_t size;

		lib::ErrorOr<std::tuple<VkBuffer, void*>> operator()(VmaWrapper& allocator) {
			ASSIGN_OR_RETURN(VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
			allocation = buffer.allocation;
			return std::make_tuple(buffer.buffer, buffer.mappedData);
		}

		lib::ErrorOr<std::tuple<VkBuffer, void*>> operator()(auto&&) {
			return lib::Error("Not recognized allocator for UniformBufferData creation");
		}
	};
};

template<typename UniformBufferType>
UniformBufferData<UniformBufferType>::UniformBufferData(const LogicalDevice& logicalDevice, uint32_t count)
	: UniformBuffer(count > 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
	_logicalDevice(logicalDevice), _count(count) {
	const PhysicalDevice& physicalDevice = _logicalDevice.getPhysicalDevice();
	const auto& limits = physicalDevice.getPropertyManager().getPhysicalDeviceLimits();
	_size = getMemoryAlignment(sizeof(UniformBufferType), limits.minUniformBufferOffsetAlignment);
	std::tie(_uniformBuffer, _uniformBufferMapped) = std::visit(Allocator{ _allocation, _count * _size }, logicalDevice.getMemoryAllocator()).value();
	
	_bufferInfo = VkDescriptorBufferInfo{
		.buffer = _uniformBuffer,
		.offset = 0,
		.range = _size
	};
}

template<typename UniformBufferType>
UniformBufferData<UniformBufferType>::~UniformBufferData() {
	if (_uniformBuffer != VK_NULL_HANDLE) {
		std::visit(BufferDeallocator{ _uniformBuffer }, _logicalDevice.getMemoryAllocator(), _allocation);
	}
}

template<typename UniformBufferType>
VkWriteDescriptorSet UniformBufferData<UniformBufferType>::getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const {
	return VkWriteDescriptorSet {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = _type,
		.pBufferInfo = &_bufferInfo
	};
}

template<typename UniformBufferType>
void UniformBufferData<UniformBufferType>::updateUniformBuffer(const UniformBufferType& object, size_t index) {
	std::memcpy(static_cast<uint8_t*>(_uniformBufferMapped) + _size * index, &object, sizeof(UniformBufferType));
}
