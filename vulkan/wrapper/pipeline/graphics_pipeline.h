#pragma once

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>

#include "pipeline.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/pipeline/shader.h"
#include "vulkan/wrapper/pipeline/shader_program.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

struct SpecializationData {
  void* data;
  size_t dataSize;
  std::map<VkShaderStageFlagBits, std::vector<VkSpecializationMapEntry>> mapEntries;
};

struct GraphicsPipelineParameters {
  VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
  std::optional<uint32_t> patchControlPoints = std::nullopt;
  float depthBiasConstantFactor = 0.0f;
  float depthBiasSlopeFactor = 0.0f;
  std::optional<SpecializationData> specializationData = std::nullopt;
};

class GraphicsPipeline : public Pipeline {
  const Renderpass& _renderpass;
  const ShaderProgram& _shaderProgram;

public:
  GraphicsPipeline(const Renderpass& renderpass, const ShaderProgram& shaderProgram,
                   const GraphicsPipelineParameters& parameters)
    : Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS), _renderpass(renderpass),
      _shaderProgram(shaderProgram) {
    const VkDevice device = _renderpass.getLogicalDevice().getVkDevice();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineTessellationStateCreateInfo tessellationState = {};
    if (parameters.patchControlPoints.has_value()) {
      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

      tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
      tessellationState.patchControlPoints = parameters.patchControlPoints.value();
    }

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = parameters.cullMode;
    // rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    if (parameters.depthBiasConstantFactor != 0.0f && parameters.depthBiasSlopeFactor != 0.0f) {
      rasterizer.depthBiasEnable = VK_TRUE;
    }
    rasterizer.depthBiasConstantFactor = parameters.depthBiasConstantFactor;
    rasterizer.depthBiasSlopeFactor = parameters.depthBiasSlopeFactor;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = renderpass.getAttachmentsLayout().getNumMsaaSamples();
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = 0.2f;

    uint32_t colorBlendAttachmentsCount =
        _renderpass.getAttachmentsLayout().getColorAttachmentsCount();
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
        colorBlendAttachmentsCount);
    for (VkPipelineColorBlendAttachmentState& colorBlendAttachment : colorBlendAttachments) {
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorBlendAttachmentsCount;
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    static constexpr VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    const lib::Buffer<VkDescriptorSetLayout> vkDescriptorSetLayouts =
        _shaderProgram.getVkDescriptorSetLayouts();
    std::span<const VkPushConstantRange> vkPushConstantRanges = _shaderProgram.getPushConstants();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = vkPushConstantRanges.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout)
        != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;  // Optional
    depthStencil.maxDepthBounds = 1.0f;  // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};  // Optional
    depthStencil.back = {};   // Optional

    lib::Buffer<VkPipelineShaderStageCreateInfo> shaders =
        _shaderProgram.getVkPipelineShaderStageCreateInfos();
    if (parameters.specializationData.has_value()) {
      for (VkPipelineShaderStageCreateInfo& shaderStage : shaders) {
        const VkShaderStageFlagBits stageFlag =
            static_cast<VkShaderStageFlagBits>(shaderStage.stage);
        auto it = parameters.specializationData->mapEntries.find(stageFlag);
        if (it != parameters.specializationData->mapEntries.cend()) {
          const VkSpecializationInfo specializationInfo = {
            .mapEntryCount = static_cast<uint32_t>(it->second.size()),
            .pMapEntries = it->second.data(),
            .dataSize = parameters.specializationData->dataSize,
            .pData = parameters.specializationData->data};
          shaderStage.pSpecializationInfo = &specializationInfo;
        }
      }
    }

    const std::optional<VkPipelineVertexInputStateCreateInfo>& vertexInputInfo =
        shaderProgram.getVkPipelineVertexInputStateCreateInfo();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaders.size());
    pipelineInfo.pStages = shaders.data();
    if (vertexInputInfo.has_value()) {
      pipelineInfo.pVertexInputState = &vertexInputInfo.value();
    }
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState =
        parameters.patchControlPoints.has_value() ? &tessellationState : nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderpass.getVkRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline)
        != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }
  }

  ~GraphicsPipeline();
};
