#pragma once

#include <logical_device/logical_device.h>
#include <memory_objects/texture/texture.h>
#include <memory_objects/buffer_deallocator.h>
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>

namespace {

VkDeviceSize getMemoryAlignment(size_t size, size_t minUboAlignment) {
	return minUboAlignment > 0 ? (size + minUboAlignment - 1) & ~(minUboAlignment - 1) : size;
}

}

class UniformBuffer {
protected:
	VkDescriptorType _type;

public:
	UniformBuffer(VkDescriptorType type) : _type(type) {}
	virtual ~UniformBuffer() = default;

	virtual VkWriteDescriptorSet getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const = 0;
	virtual uint32_t getSize() const = 0;

	VkDescriptorType getVkDescriptorType() const { return _type; }
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

	uint32_t getSize() const override {
		// TODO
		return 0;
	}
};

template<typename UniformBufferType>
class UniformBufferData : public UniformBuffer {
	Allocation _allocation;
	void* _uniformBufferMapped;

	VkDescriptorBufferInfo _bufferInfo;
	uint32_t _count;

	const LogicalDevice& _logicalDevice;

	UniformBufferData(const LogicalDevice& logicalDevice, const VkBuffer uniformBuffer, VkDescriptorType type, const Allocation allocation, uint32_t count, uint32_t size, void* mappedBuffer);

public:
	static ErrorOr<std::unique_ptr<UniformBufferData>> create(const LogicalDevice& logicalDevice, uint32_t count = 1);

	~UniformBufferData() override;

	VkWriteDescriptorSet getVkWriteDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const override;
	uint32_t getSize() const override;

	void updateUniformBuffer(const UniformBufferType& object, size_t index = 0);

private:
	struct Allocator {
		Allocation& allocation;
		const size_t size;

		ErrorOr<std::pair<VkBuffer, void*>> operator()(VmaWrapper& allocator) {
			ASSIGN_OR_RETURN(VmaWrapper::Buffer buffer, allocator.createVkBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT));
			allocation = buffer.allocation;
			return std::make_pair(buffer.buffer, buffer.mappedData);
		}

		ErrorOr<std::pair<VkBuffer, void*>> operator()(auto&&) {
			return Error(EngineError::NOT_RECOGNIZED_TYPE);
		}
	};
};

template<typename UniformBufferType>
UniformBufferData<UniformBufferType>::UniformBufferData(const LogicalDevice& logicalDevice, const VkBuffer uniformBuffer, VkDescriptorType type, const Allocation allocation, uint32_t count, uint32_t size, void* mappedBuffer)
	: UniformBuffer(type), _allocation(allocation), _uniformBufferMapped(mappedBuffer), _bufferInfo{uniformBuffer, 0, size}, _count(count), _logicalDevice(logicalDevice) {
}

template<typename UniformBufferType>
ErrorOr<std::unique_ptr<UniformBufferData<UniformBufferType>>> UniformBufferData<UniformBufferType>::create(const LogicalDevice& logicalDevice, uint32_t count) {
	const auto& limits = logicalDevice.getPhysicalDevice().getPropertyManager().getPhysicalDeviceLimits();
	const uint32_t size = getMemoryAlignment(sizeof(UniformBufferType), limits.minUniformBufferOffsetAlignment);
	Allocation allocation;
	ASSIGN_OR_RETURN(const std::pair buffer, std::visit(Allocator{ allocation, count * size }, logicalDevice.getMemoryAllocator()));
	return std::unique_ptr<UniformBufferData<UniformBufferType>>(new UniformBufferData<UniformBufferType>(logicalDevice, buffer.first, count > 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, allocation, count, size, buffer.second));
}

template<typename UniformBufferType>
UniformBufferData<UniformBufferType>::~UniformBufferData() {
	std::visit(BufferDeallocator{ _bufferInfo.buffer }, _logicalDevice.getMemoryAllocator(), _allocation);
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
uint32_t UniformBufferData<UniformBufferType>::getSize() const {
	return _bufferInfo.range;
}

template<typename UniformBufferType>
void UniformBufferData<UniformBufferType>::updateUniformBuffer(const UniformBufferType& object, size_t index) {
	std::memcpy(static_cast<uint8_t*>(_uniformBufferMapped) + _bufferInfo.range * index, &object, sizeof(UniformBufferType));
}
