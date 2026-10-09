#include "vulkan/wrapper/pipeline/compute_pipeline_builder.h"

#include <cassert>
#include <optional>
#include <vulkan/vulkan.h>

#include "vulkan/wrapper/pipeline/pipeline_layout.h"

Pipeline ComputePipelineBuilder::createPipeline(const PipelineLayout& pipelineLayout) {
  assert(_shaderStage.has_value());
  const VkComputePipelineCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = *_shaderStage,
    .layout = pipelineLayout.getVkPipelineLayout()};
  return Pipeline::createComputePipeline(pipelineLayout.getLogicalDevice(), createInfo);
}

ComputePipelineBuilder& ComputePipelineBuilder::withShaderStageCreateInfo(
    const VkPipelineShaderStageCreateInfo& shaderStage) {
  _shaderStage = shaderStage;
  return *this;
}
