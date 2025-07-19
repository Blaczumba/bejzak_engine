#include "command_buffer.h"

#include <stdexcept>

CommandPool::CommandPool(const LogicalDevice& logicalDevice, VkCommandPool commandPool) : _logicalDevice(logicalDevice), _commandPool(commandPool) {}

ErrorOr<std::unique_ptr<CommandPool>> CommandPool::create(const LogicalDevice& logicalDevice) {
    const VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = *logicalDevice.getPhysicalDevice().getQueueFamilyIndices().graphicsFamily
    };

	VkCommandPool commandPool;
    if (VkResult result = vkCreateCommandPool(logicalDevice.getVkDevice(), &poolInfo, nullptr, &commandPool); result != VK_SUCCESS) {
        return Error(result);
    }
	return std::unique_ptr<CommandPool>(new CommandPool(logicalDevice, commandPool));
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(_logicalDevice.getVkDevice(), _commandPool, nullptr);
}
std::unique_ptr<PrimaryCommandBuffer> CommandPool::createPrimaryCommandBuffer() const {
    return std::make_unique<PrimaryCommandBuffer>(shared_from_this());
}

std::unique_ptr<SecondaryCommandBuffer> CommandPool::createSecondaryCommandBuffer() const {
    return std::make_unique<SecondaryCommandBuffer>(shared_from_this());
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

CommandBuffer::CommandBuffer(const std::shared_ptr<const CommandPool>& commandPool, VkCommandBufferLevel level)
    :_commandPool(commandPool) {
    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _commandPool->getVkCommandPool(),
        .level = level,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(_commandPool->getLogicalDevice().getVkDevice(), &allocInfo, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(_commandPool->getLogicalDevice().getVkDevice(), _commandPool->getVkCommandPool(), 1, &_commandBuffer);
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

PrimaryCommandBuffer::PrimaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool)
    : CommandBuffer(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY) {}

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

SecondaryCommandBuffer::SecondaryCommandBuffer(const std::shared_ptr<const CommandPool>& commandPool)
    : CommandBuffer(commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY) {}

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

    if (vkCreateFence(device, &fenceInfo, nullptr, &_fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create SingleTimeCommandBuffer fence!");
    }

    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _commandPool.getVkCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &allocInfo, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

SingleTimeCommandBuffer::~SingleTimeCommandBuffer() {
    const LogicalDevice& logicalDevice = _commandPool.getLogicalDevice();
    const VkDevice device = logicalDevice.getVkDevice();
    const VkQueue queue = logicalDevice.getVkQueue(_queueType);

    vkEndCommandBuffer(_commandBuffer);

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &_commandBuffer
    };

    vkQueueSubmit(queue, 1, &submitInfo, _fence);
    vkWaitForFences(device, 1, &_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, _fence, nullptr);

    vkFreeCommandBuffers(device, _commandPool.getVkCommandPool(), 1, &_commandBuffer);
}

VkCommandBuffer SingleTimeCommandBuffer::getCommandBuffer() const {
    return _commandBuffer;
}
