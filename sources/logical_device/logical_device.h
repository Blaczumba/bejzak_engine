#pragma once

#include "memory_allocator/allocation.h"
#include "memory_allocator/memory_allocator.h"
#include "memory_objects/texture.h"
#include "physical_device/physical_device.h"
#include "status/status.h"

#include <memory>
#include <variant>

enum class QueueType : uint8_t {
	GRAPHICS = 0,
	PRESENT,
	COMPUTE,
	TRANSFER
};

class LogicalDevice {
	VkDevice _device;

	const PhysicalDevice& _physicalDevice;
	mutable MemoryAllocator _memoryAllocator;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;
	VkQueue _computeQueue;
	VkQueue _transferQueue;

	LogicalDevice(const VkDevice logicalDevice, const PhysicalDevice& physicalDevice, const VkQueue graphicsQueue, const VkQueue presentQueue, const VkQueue computeQueue, const VkQueue transferQueue);

public:
	~LogicalDevice();

	static ErrorOr<std::unique_ptr<LogicalDevice>> create(const PhysicalDevice& physicalDevice);

	VkSampler createSampler(const SamplerParameters& params) const;
	VkImageView createImageView(const VkImage image, const ImageParameters& params) const;

	VkDevice getVkDevice() const;
	const PhysicalDevice& getPhysicalDevice() const;
	MemoryAllocator& getMemoryAllocator() const;

	VkQueue getVkQueue(QueueType queueType) const;
	VkQueue getGraphicsVkQueue() const;
	VkQueue getPresentVkQueue() const;
	VkQueue getComputeVkQueue() const;
	VkQueue getTransferVkQueue() const;
};
