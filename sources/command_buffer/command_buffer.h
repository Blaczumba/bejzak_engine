#pragma once

#include <vulkan/vulkan.h>

#include "logical_device/logical_device.h"

#include <memory>

class PrimaryCommandBuffer;
class SecondaryCommandBuffer;
class CommandBuffer;
class Framebuffer;

class CommandPool {
	VkCommandPool _commandPool;
	const LogicalDevice& _logicalDevice;

public:
	CommandPool(const LogicalDevice& logicalDevice);
	~CommandPool();

	std::unique_ptr<PrimaryCommandBuffer> createPrimaryCommandBuffer() const;
	std::unique_ptr<SecondaryCommandBuffer> createSecondaryCommandBuffer() const;
	void reset() const;

	VkCommandPool getVkCommandPool() const;
	const LogicalDevice& getLogicalDevice() const;
};

class CommandBuffer {
protected:
	VkCommandBuffer _commandBuffer;
	const CommandPool& _commandPool;

public:
	CommandBuffer(const CommandPool& commandPool, VkCommandBufferLevel level);
	~CommandBuffer();
	VkResult end() const;
	void resetCommandBuffer() const;
	VkCommandBuffer getVkCommandBuffer() const;
};

class PrimaryCommandBuffer : public CommandBuffer {
	VkViewport _viewport;
	VkRect2D _scissor;
public:
	PrimaryCommandBuffer(const CommandPool& commandPool);
	VkResult begin(uint32_t subpassIndex = 0) const;
	void beginRenderPass(const Framebuffer& framebuffer) const;
	void endRenderPass() const;
	void executeSecondaryCommandBuffers(std::initializer_list<VkCommandBuffer> commandBuffers) const;
	VkResult submit(QueueType type, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore, const VkFence waitFence) const;
};

class SecondaryCommandBuffer : public CommandBuffer {
public:
	SecondaryCommandBuffer(const CommandPool& commandPool);
	VkResult begin(const Framebuffer& framebuffer, const VkCommandBufferInheritanceViewportScissorInfoNV* scissorViewportInheritance = nullptr, uint32_t subpassIndex = 0) const;
};

class SingleTimeCommandBuffer {
	VkCommandBuffer _commandBuffer;
	VkFence _fence;
	const QueueType _queueType;

	const CommandPool& _commandPool;

public:
	SingleTimeCommandBuffer(const CommandPool& commandPool, QueueType queueType = QueueType::GRAPHICS);
	~SingleTimeCommandBuffer();

	VkCommandBuffer getCommandBuffer() const;
};
