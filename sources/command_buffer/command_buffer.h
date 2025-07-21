#pragma once

#include "logical_device/logical_device.h"
#include "framebuffer/framebuffer.h"
#include "status/status.h"

#include <vulkan/vulkan.h>

#include <memory>

class PrimaryCommandBuffer;
class SecondaryCommandBuffer;
class CommandBuffer;

class CommandPool : public std::enable_shared_from_this<const CommandPool> {
	VkCommandPool _commandPool;
	const LogicalDevice& _logicalDevice;

	CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool);
public:
	static ErrorOr<std::unique_ptr<CommandPool>> create(const LogicalDevice& logicalDevice);

	~CommandPool();

	ErrorOr<std::unique_ptr<PrimaryCommandBuffer>> createPrimaryCommandBuffer() const;

	ErrorOr<std::vector<std::unique_ptr<PrimaryCommandBuffer>>> createPrimaryCommandBuffers(uint32_t count) const;

	ErrorOr<std::unique_ptr<SecondaryCommandBuffer>> createSecondaryCommandBuffer() const;

	ErrorOr<std::vector<std::unique_ptr<SecondaryCommandBuffer>>> createSecondaryCommandBuffers(uint32_t count) const;

	void reset() const;

	VkCommandPool getVkCommandPool() const;

	const LogicalDevice& getLogicalDevice() const;
};

class CommandBuffer {
protected:
	VkCommandBuffer _commandBuffer;
	std::shared_ptr<const CommandPool> _commandPool;

	CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

public:
	CommandBuffer(CommandBuffer&&) noexcept;

	CommandBuffer& operator=(CommandBuffer&&) noexcept;

	~CommandBuffer();

	VkResult end() const;

	void resetCommandBuffer() const;

	VkCommandBuffer getVkCommandBuffer() const;

};

class PrimaryCommandBuffer : public CommandBuffer {
	PrimaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

public:
	PrimaryCommandBuffer(PrimaryCommandBuffer&&) noexcept = default;

	PrimaryCommandBuffer& operator=(PrimaryCommandBuffer&&) noexcept = default;

	static ErrorOr<std::unique_ptr<PrimaryCommandBuffer>> create(const std::shared_ptr<const CommandPool>& commandPool);

	static ErrorOr<std::vector<std::unique_ptr<PrimaryCommandBuffer>>> create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count);

	VkResult begin(uint32_t subpassIndex = 0) const;

	void beginRenderPass(const Framebuffer& framebuffer) const;

	void endRenderPass() const;

	void executeSecondaryCommandBuffers(std::initializer_list<VkCommandBuffer> commandBuffers) const;

	VkResult submit(QueueType type, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore, const VkFence waitFence) const;
};

class SecondaryCommandBuffer : public CommandBuffer {
	SecondaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

public:
	SecondaryCommandBuffer(SecondaryCommandBuffer&&) noexcept = default;

	SecondaryCommandBuffer& operator=(SecondaryCommandBuffer&&) noexcept = default;

	static ErrorOr<std::unique_ptr<SecondaryCommandBuffer>> create(const std::shared_ptr<const CommandPool>& commandPool);

	static ErrorOr<std::vector<std::unique_ptr<SecondaryCommandBuffer>>> create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count);

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
