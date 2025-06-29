#pragma once

#include "pipeline.h"

#include "logical_device/logical_device.h"
#include "memory_objects/uniform_buffer/push_constants.h"
#include "primitives/vk_primitives.h"
#include "render_pass/render_pass.h"
#include "shader/shader.h"
#include "shader/shader_program.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>

struct GraphicsPipelineParameters {
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    std::optional<uint32_t> patchControlPoints = std::nullopt;
    float depthBiasConstantFactor = 0.0f;
    float depthBiasSlopeFactor = 0.0f;
};

class GraphicsPipeline : public Pipeline {
    GraphicsPipelineParameters _parameters;

    const Renderpass& _renderpass;
    const ShaderProgram& _shaderProgram;

public:
    GraphicsPipeline(const Renderpass& renderpass, const GraphicsShaderProgram& shaderProgram, const GraphicsPipelineParameters& parameters)
        : Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS), _renderpass(renderpass), _shaderProgram(shaderProgram), _parameters(parameters) {
        const VkDevice device = _renderpass.getLogicalDevice().getVkDevice();
        std::span<const Shader> shaders = _shaderProgram.getShaders();
        lib::Buffer<VkPipelineShaderStageCreateInfo> shaderStages(shaders.size());
        std::transform(shaders.cbegin(), shaders.cend(), shaderStages.begin(), [](const Shader& shader) { return shader.getVkPipelineStageCreateInfo(); });

        const VkPipelineVertexInputStateCreateInfo& vertexInputInfo = shaderProgram.getVkPipelineVertexInputStateCreateInfo();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineTessellationStateCreateInfo tessellationState = {};
        if (_parameters.patchControlPoints.has_value()) {
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

            tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tessellationState.patchControlPoints = _parameters.patchControlPoints.value();
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
        rasterizer.cullMode = _parameters.cullMode;
        // rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        if (_parameters.depthBiasConstantFactor != 0.0f && _parameters.depthBiasSlopeFactor != 0.0f)
            rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = _parameters.depthBiasConstantFactor;
        rasterizer.depthBiasSlopeFactor = _parameters.depthBiasSlopeFactor;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = _parameters.msaaSamples;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.minSampleShading = 0.2f;

        uint32_t colorBlendAttachmentsCount = _renderpass.getAttachmentsLayout().getColorAttachmentsCount();
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorBlendAttachmentsCount);
        for (VkPipelineColorBlendAttachmentState& colorBlendAttachment : colorBlendAttachments) {
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        std::span<const DescriptorSetLayout> descriptorSetLayouts = _shaderProgram.getDescriptorSetLayouts();
        lib::Buffer<VkDescriptorSetLayout> vkDescriptorSetLayouts(descriptorSetLayouts.size());
        std::transform(descriptorSetLayouts.cbegin(), descriptorSetLayouts.cend(), vkDescriptorSetLayouts.begin(), [](const DescriptorSetLayout& layout) { return layout.getVkDescriptorSetLayout(); });
        std::span<const VkPushConstantRange> pushConstants = _shaderProgram.getPushConstants();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkDescriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
        if (!pushConstants.empty()) {
            pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
        }

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pTessellationState = _parameters.patchControlPoints.has_value() ? &tessellationState : nullptr;
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

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

	~GraphicsPipeline();
};

