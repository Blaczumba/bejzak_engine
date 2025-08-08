#include "command_buffer.h"

#include "vulkan_wrapper/util/check.h"

#include <iterator>

CommandPool::CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool) : _logicalDevice(logicalDevice), _commandPool(commandPool) {}

ErrorOr<std::unique_ptr<CommandPool>> CommandPool::create(const LogicalDevice& logicalDevice) {
    const VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *logicalDevice.getPhysicalDevice().getQueueFamilyIndices().graphicsFamily
    };

	VkCommandPool commandPool;
	CHECK_VKCMD(vkCreateCommandPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &commandPool));
	return std::unique_ptr<CommandPool>(new CommandPool(logicalDevice, commandPool));
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(_logicalDevice.getVkDevice(), _commandPool, nullptr);
}
ErrorOr<PrimaryCommandBuffer> CommandPool::createPrimaryCommandBuffer() const {
    return PrimaryCommandBuffer::create(shared_from_this());
}

ErrorOr<std::vector<PrimaryCommandBuffer>> CommandPool::createPrimaryCommandBuffers(uint32_t count) const {
    return PrimaryCommandBuffer::create(shared_from_this(), count);
}

ErrorOr<SecondaryCommandBuffer> CommandPool::createSecondaryCommandBuffer() const {
    return SecondaryCommandBuffer::create(shared_from_this());
}

ErrorOr<std::vector<SecondaryCommandBuffer>> CommandPool::createSecondaryCommandBuffers(uint32_t count) const {
    return SecondaryCommandBuffer::create(shared_from_this(), count);
}

void CommandPool::reset() const {
    vkResetCommandPool(_logicalDevice.getVkDevice(), _commandPool, 0);
}

VkCommandPool CommandPool::getVkCommandPool() const {
    return _commandPool;
}

const LogicalDevice& CommandPool::getLogicalDevice() const {
    return _logicalDevice;
}

CommandBuffer::CommandBuffer() : _commandBuffer(VK_NULL_HANDLE) { }

CommandBuffer::CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer)
    :_commandPool(commandPool), _commandBuffer(commandBuffer) {}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
	: _commandPool(std::move(other._commandPool)), _commandBuffer(std::exchange(other._commandBuffer, VK_NULL_HANDLE)) {}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept {
	if (this == &other) {
	    return *this;
	}
    _commandPool = std::move(other._commandPool);
	_commandBuffer = std::exchange(other._commandBuffer, VK_NULL_HANDLE);
    return *this;
}

CommandBuffer::~CommandBuffer() {
	if (_commandBuffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(_commandPool->getLogicalDevice().getVkDevice(), _commandPool->getVkCommandPool(), 1, &_commandBuffer);
	}
}

VkResult CommandBuffer::end() const {
    return vkEndCommandBuffer(_commandBuffer);
}

void CommandBuffer::resetCommandBuffer() const {
    vkResetCommandBuffer(_commandBuffer, 0);
}

VkCommandBuffer CommandBuffer::getVkCommandBuffer() const {
    return _commandBuffer;
}

PrimaryCommandBuffer::PrimaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer)
    : CommandBuffer(commandPool, commandBuffer) {}

ErrorOr<PrimaryCommandBuffer> PrimaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool) {
    VkCommandBuffer commandBuffer;
    CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, { &commandBuffer, 1 }));
    return PrimaryCommandBuffer(commandPool, commandBuffer);
}

namespace {

template<typename CommandBufferType>
std::vector<CommandBufferType> transformCommandBuffers(std::span<const VkCommandBuffer> inCommandBuffers, const std::shared_ptr<const CommandPool>& commandPool) {
    std::vector<CommandBufferType> commandBuffers;
	commandBuffers.reserve(inCommandBuffers.size());
    std::transform(std::cbegin(inCommandBuffers), std::cend(inCommandBuffers), std::back_inserter(commandBuffers),
        [&commandPool](VkCommandBuffer commandBuffer) {
        return CommandBufferType(commandPool, commandBuffer);
    });
    return commandBuffers;
}

} // namespace

ErrorOr<std::vector<PrimaryCommandBuffer>> PrimaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count) {
    lib::Buffer<VkCommandBuffer> commandBuffers(count);
	CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffers));
    return transformCommandBuffers<PrimaryCommandBuffer>(commandBuffers, commandPool);
}

VkResult PrimaryCommandBuffer::begin(uint32_t subpassIndex) const {
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    return vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

void PrimaryCommandBuffer::beginRenderPass(const Framebuffer& framebuffer) const {
    const Renderpass& renderpass = framebuffer.getRenderpass();
    std::span<const VkClearValue> clearValues = renderpass.getAttachmentsLayout().getVkClearValues();
    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderpass.getVkRenderPass(),
        .framebuffer = framebuffer.getVkFramebuffer(),
        .renderArea = {
            .offset = { 0, 0 },
            .extent = framebuffer.getVkExtent()
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };
    vkCmdSetViewport(_commandBuffer, 0, 1, &framebuffer.getViewport());
    vkCmdSetScissor(_commandBuffer, 0, 1, &framebuffer.getScissor());
    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void PrimaryCommandBuffer::endRenderPass() const {
    vkCmdEndRenderPass(_commandBuffer);
}

void PrimaryCommandBuffer::executeSecondaryCommandBuffers(std::initializer_list<VkCommandBuffer> commandBuffers) const {
    vkCmdExecuteCommands(_commandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.begin());
}

VkResult PrimaryCommandBuffer::submit(QueueType type, const VkSemaphore waitSemaphore, const VkSemaphore signalSemaphore, const VkFence waitFence) const {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { waitSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    if (waitSemaphore != VK_NULL_HANDLE) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    VkSemaphore signalSemaphores[] = { signalSemaphore };
    if (signalSemaphore != VK_NULL_HANDLE) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
    }

    const LogicalDevice& logicalDevice = _commandPool->getLogicalDevice();
    if (waitFence != VK_NULL_HANDLE) {
        vkResetFences(logicalDevice.getVkDevice(), 1, &waitFence);
    }

    return vkQueueSubmit(logicalDevice.getVkQueue(type), 1, &submitInfo, waitFence);
}

SecondaryCommandBuffer::SecondaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBuffer commandBuffer)
    : CommandBuffer(commandPool, commandBuffer) {}

ErrorOr<SecondaryCommandBuffer> SecondaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool) {
    VkCommandBuffer commandBuffer;
	CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_SECONDARY, { &commandBuffer, 1 }));
    return SecondaryCommandBuffer(commandPool, commandBuffer);
}

ErrorOr<std::vector<SecondaryCommandBuffer>> SecondaryCommandBuffer::create(const std::shared_ptr<const CommandPool>& commandPool, uint32_t count) {
    lib::Buffer<VkCommandBuffer> commandBuffers(count);
	CHECK_VKCMD(createCommandBuffers(commandPool->getLogicalDevice().getVkDevice(), commandPool->getVkCommandPool(), VK_COMMAND_BUFFER_LEVEL_SECONDARY, commandBuffers));
    return transformCommandBuffers<SecondaryCommandBuffer>(commandBuffers, commandPool);
}

VkResult SecondaryCommandBuffer::begin(const Framebuffer& framebuffer, const VkCommandBufferInheritanceViewportScissorInfoNV* scissorViewportInheritance, uint32_t subpassIndex) const {
    const VkCommandBufferInheritanceInfo inheritance = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext = scissorViewportInheritance,
        .renderPass = framebuffer.getRenderpass().getVkRenderPass(),
        .subpass = subpassIndex,
        .framebuffer = framebuffer.getVkFramebuffer()
    };
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance
    };
    return vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

SingleTimeCommandBuffer::SingleTimeCommandBuffer(const CommandPool& commandPool, QueueType queueType)
    : _commandPool(commandPool), _queueType(queueType) {
    const VkDevice device = _commandPool.getLogicalDevice().getVkDevice();
    const VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };

    vkCreateFence(device, &fenceInfo, nullptr, &_fence);

    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _commandPool.getVkCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device, &allocInfo, &_commandBuffer);

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer() {
    const LogicalDevice& logicalDevice = _commandPool.getLogicalDevice();
    const VkDevice device = logicalDevice.getVkDevice();

    vkEndCommandBuffer(_commandBuffer);

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer
    };

    vkQueueSubmit(logicalDevice.getVkQueue(_queueType), 1, &submitInfo, _fence);
    vkWaitForFences(device, 1, &_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, _fence, nullptr);

    vkFreeCommandBuffers(device, _commandPool.getVkCommandPool(), 1, &_commandBuffer);
}

VkCommandBuffer SingleTimeCommandBuffer::getCommandBuffer() const {
    return _commandBuffer;
}
