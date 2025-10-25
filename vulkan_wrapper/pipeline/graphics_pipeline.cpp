#include "graphics_pipeline.h"

GraphicsPipeline::~GraphicsPipeline() {
  const VkDevice device = _renderpass.getLogicalDevice().getVkDevice();

  vkDestroyPipeline(device, _pipeline, nullptr);
  vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
}
