#include "compute_pipeline.h"

#include <stdexcept>

ComputePipeline::ComputePipeline(const LogicalDevice& logicalDevice, VkDescriptorSetLayout descriptorSetLayout, const std::string& computeShader)
    : Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE), _logicalDevice(logicalDevice) {
    /*const VkDevice device = _logicalDevice.getVkDevice();
    
    const auto shader = Shader::create(logicalDevice, "", VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader->getVkPipelineStageCreateInfo(),
        .layout = _pipelineLayout
    };

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }*/
}

ComputePipeline::~ComputePipeline() {
    VkDevice device = _logicalDevice.getVkDevice();

    vkDestroyPipeline(device, _pipeline, nullptr);
    vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
}
