#pragma once

#include <vulkan/vulkan.h>

struct PipelineStageInfo {
  VkAccessFlags accessFlags;
  VkPipelineStageFlags stageFlags;
};

PipelineStageInfo sourceStageAndAccessMask(VkImageLayout layout);

PipelineStageInfo destinationStageAndAccessMask(VkImageLayout layout);
