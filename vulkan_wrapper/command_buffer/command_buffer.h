#pragma once

#include "common/status/status.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/framebuffer/framebuffer.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <span>

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

	ErrorOr<PrimaryCommandBuffer> createPrimaryCommandBuffer() const;

	ErrorOr<std::vector<PrimaryCommandBuffer>> createPrimaryCommandBuffers(uint32_t count) const;

	template<size_t COUNT>
	ErrorOr<std::array<PrimaryCommandBuffer, COUNT>> createPrimaryCommandBuffers() const;

	ErrorOr<SecondaryCommandBuffer> createSecondaryCommandBuffer() const;

	template<size_t COUNT>
	ErrorOr<std::array<SecondaryCommandBuffer, COUNT>> createSecondaryCommandBuffers() const;

	ErrorOr<std::vector<SecondaryCommandBuffer>> createSecondaryCommandBuffers(uint32_t count) const;

	void reset() const;

	VkCommandPool getVkCommandPool() const;

	const LogicalDevice& getLogicalDevice() const;
};

class CommandBuffer {
protected:
	VkCommandBuffer _commandBuffer;
	std::shared_ptr<const CommandPool> _commandPool;

	CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

	CommandBuffer();

public:
	CommandBuffer(CommandBuffer&&) noexcept;

	CommandBuffer& operator=(CommandBuffer&&) noexcept;

	~CommandBuffer();

	VkResult end() const;

	void resetCommandBuffer() const;

	VkCommandBuffer getVkCommandBuffer() const;

};

class PrimaryCommandBuffer : public CommandBuffer {
public:
	PrimaryCommandBuffer(PrimaryCommandBuffer&&) noexcept = default;

	PrimaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

	PrimaryCommandBuffer& operator=(PrimaryCommandBuffer&&) noexcept = default;

	static ErrorOr<PrimaryCommandBuffer> create(const std::shared_ptr<const CommandPool>& commandPool);

	template<size_t COUNT>
	static ErrorOr<std::array<PrimaryCommandBuffer, COUNT>> create(const std::shared_ptr<const CommandPool>& commandPool);

	static ErrorOr<std::vector<PrimaryCommandBuffer>> create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count);

	VkResult begin(uint32_t subpassIndex = 0) const;

	void beginRenderPass(const Framebuffer& framebuffer) const;

	void endRenderPass() const;

	void executeSecondaryCommandBuffers(std::initializer_list<VkCommandBuffer> commandBuffers) const;

	VkResult submit(QueueType type, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore, const VkFence waitFence) const;
};

class SecondaryCommandBuffer : public CommandBuffer {
public:
	SecondaryCommandBuffer() = default;

	SecondaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer);

	SecondaryCommandBuffer(SecondaryCommandBuffer&&) noexcept = default;

	SecondaryCommandBuffer& operator=(SecondaryCommandBuffer&&) noexcept = default;

	static ErrorOr<SecondaryCommandBuffer> create(const std::shared_ptr<const CommandPool>& commandPool);

	template<size_t COUNT>
	static ErrorOr<std::array<SecondaryCommandBuffer, COUNT>> create(const std::shared_ptr<const CommandPool>& commandPool);

	static ErrorOr<std::vector<SecondaryCommandBuffer>> create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count);

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

namespace {

VkResult createCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBufferLevel level, std::span<VkCommandBuffer> outCommandBuffers) {
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = level,
		.commandBufferCount = static_cast<uint32_t>(outCommandBuffers.size()),
	};
	return vkAllocateCommandBuffers(device, &allocInfo, outCommandBuffers.data());
}

template<typename CommandBufferType, size_t COUNT>
std::array<CommandBufferType, COUNT> transformCommandBuffers(std::span<const VkCommandBuffer> inCommandBuffers, const std::shared_ptr<const CommandPool>& commandPool) {
	std::array<CommandBufferType, COUNT> commandBuffers;
	std::transform(std::cbegin(inCommandBuffers), std::cend(inCommandBuffers), commandBuffers.begin(),
		[&commandPool](VkCommandBuffer commandBuffer) {
		return CommandBufferType(commandPool, commandBuffer);
	});
	return commandBuffers;
}

} // namespace

template<size_t COUNT>
ErrorOr<std::array<PrimaryCommandBuffer, COUNT>> PrimaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool) {
	std::array<VkCommandBuffer, COUNT> commandBuffers;
	if (VkResult result = createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffers); result != VK_SUCCESS) {
		return Error(result);
	}
	return transformCommandBuffers<PrimaryCommandBuffer, COUNT>(commandBuffers, commandPool);
}

template<size_t COUNT>
ErrorOr<std::array<SecondaryCommandBuffer, COUNT>> SecondaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool) {
	std::array<VkCommandBuffer, COUNT> commandBuffers;
	if (VkResult result = createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_SECONDARY, commandBuffers); result != VK_SUCCESS) {
		return Error(result);
	}
	return transformCommandBuffers<SecondaryCommandBuffer, COUNT>(commandBuffers, commandPool);
}

template<size_t COUNT>
ErrorOr<std::array<PrimaryCommandBuffer, COUNT>> CommandPool::createPrimaryCommandBuffers() const {
	return PrimaryCommandBuffer::create<COUNT>(shared_from_this());
}

template<size_t COUNT>
ErrorOr<std::array<SecondaryCommandBuffer, COUNT>> CommandPool::createSecondaryCommandBuffers() const {
	return SecondaryCommandBuffer::create<COUNT>(shared_from_this());
}
