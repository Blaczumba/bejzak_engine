#include "buffers.h"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <variant>

namespace {

struct PipelineStageInfo {
    VkAccessFlags accessFlags;
    VkPipelineStageFlags stageFlags;
};

constexpr PipelineStageInfo sourceStageAndAccessMask(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return { 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return { VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return { VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    default:
        return { 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
    }
}

constexpr PipelineStageInfo destinationStageAndAccessMask(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_GENERAL:
        return { VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return { VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return { VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT };
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT };
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return { VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    default:
        return { 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
    }
}

}

void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layerCount) {
    const VkImageSubresourceRange range = {
        .aspectMask = aspectFlags,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = layerCount
    };

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = range
    };

    const PipelineStageInfo srcStageInfo = sourceStageAndAccessMask(oldLayout);
    const PipelineStageInfo dstStageInfo = sourceStageAndAccessMask(newLayout);
    barrier.srcAccessMask = srcStageInfo.accessFlags;
    barrier.dstAccessMask = dstStageInfo.accessFlags;

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageInfo.stageFlags, dstStageInfo.stageFlags,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void copyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) {
    const VkBufferCopy copyRegion = {
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        },
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const std::vector<VkBufferImageCopy>& regions) {
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(regions.size()),
        regions.data()
    );
}

void copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout layout, VkBuffer buffer, uint32_t width, uint32_t height) {
    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        },
    };

    vkCmdCopyImageToBuffer(commandBuffer, image, layout, buffer, 1, &region);
}

void copyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, VkExtent2D srcSize, VkExtent2D dstSize, VkImageAspectFlagBits aspect) {
    const VkOffset3D srcBlitSize = {
        .x = static_cast<int32_t>(srcSize.width),
        .y = static_cast<int32_t>(srcSize.height),
        .z = 1
    };

    const VkOffset3D dstBlitSize = {
        .x = static_cast<int32_t>(dstSize.width),
        .y = static_cast<int32_t>(dstSize.height),
        .z = 1
    };

    const VkImageBlit imageBlitRegion = {
        .srcSubresource = {
            .aspectMask = static_cast<VkImageAspectFlags>(aspect),
            .layerCount = 1,
        },
        .srcOffsets = {
            {},
            srcBlitSize,
        },
        .dstSubresource = {
            .aspectMask = static_cast<VkImageAspectFlags>(aspect),
            .layerCount = 1,
        },
        .dstOffsets = {
            {},
            dstBlitSize
        }
    };

    vkCmdBlitImage(
        commandBuffer,
        srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageBlitRegion,
        VK_FILTER_LINEAR);
}

void copyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, VkExtent2D extent, VkImageAspectFlagBits aspect) {
    const VkImageCopy imageCopyRegion = {
        .srcSubresource = {
            .aspectMask = static_cast<VkImageAspectFlags>(aspect),
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = static_cast<VkImageAspectFlags>(aspect),
            .layerCount = 1
        },
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        }
    };

    vkCmdCopyImage(
        commandBuffer,
        srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion);
}

// Image layout needs to be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
void generateImageMipmaps(VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, VkImageLayout finalLayout, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = layerCount,
        }
    };

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        {
            constexpr PipelineStageInfo srcStageInfo = sourceStageAndAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            constexpr PipelineStageInfo dstStageInfo = destinationStageAndAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = srcStageInfo.accessFlags;
            barrier.dstAccessMask = dstStageInfo.accessFlags;
            vkCmdPipelineBarrier(commandBuffer,
                srcStageInfo.stageFlags, dstStageInfo.stageFlags, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        const VkImageBlit blit = {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = layerCount
            },
            .srcOffsets = { { 0, 0, 0 }, { mipWidth, mipHeight, 1 } },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = layerCount
            },
            .dstOffsets = { { 0, 0, 0 }, { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 } },
        };
        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        constexpr PipelineStageInfo srcStageInfo = sourceStageAndAccessMask(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        const PipelineStageInfo dstStageInfo = destinationStageAndAccessMask(finalLayout);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = finalLayout;
        barrier.srcAccessMask = srcStageInfo.accessFlags;
        barrier.dstAccessMask = dstStageInfo.accessFlags;

        vkCmdPipelineBarrier(commandBuffer,
            srcStageInfo.stageFlags, dstStageInfo.stageFlags, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    constexpr PipelineStageInfo srcStageInfo = sourceStageAndAccessMask(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    const PipelineStageInfo dstStageInfo = destinationStageAndAccessMask(finalLayout);
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = finalLayout;
    barrier.srcAccessMask = srcStageInfo.accessFlags;
    barrier.dstAccessMask = dstStageInfo.accessFlags;
    vkCmdPipelineBarrier(commandBuffer,
        srcStageInfo.stageFlags, dstStageInfo.stageFlags, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}
