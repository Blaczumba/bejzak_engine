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

	const VkSampler createSampler(const SamplerParameters& params) const;
	const VkImageView createImageView(const VkImage image, const ImageParameters& params) const;

	const VkDevice getVkDevice() const;
	const PhysicalDevice& getPhysicalDevice() const;
	MemoryAllocator& getMemoryAllocator() const;

	const VkQueue getQueue(QueueType queueType) const;
	const VkQueue getGraphicsQueue() const;
	const VkQueue getPresentQueue() const;
	const VkQueue getComputeQueue() const;
	const VkQueue getTransferQueue() const;
};
