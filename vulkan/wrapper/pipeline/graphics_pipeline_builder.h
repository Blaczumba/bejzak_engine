#pragma once

#include <array>
#include <initializer_list>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/wrapper/pipeline/pipeline.h"
#include "vulkan/wrapper/pipeline/pipeline_layout.h"
#include "vulkan/wrapper/render_pass/render_pass.h"

class GraphicsPipelineBuilder {
public:
  GraphicsPipelineBuilder(
      lib::Buffer<VkDynamicState>&& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);

  GraphicsPipelineBuilder(std::initializer_list<VkDynamicState> dynamicStates,
                          VkPipelineDynamicStateCreateFlags flags = 0);

  GraphicsPipelineBuilder(
      std::span<const VkDynamicState> dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);

  GraphicsPipelineBuilder(const GraphicsPipelineBuilder& other) = delete;

  GraphicsPipelineBuilder(GraphicsPipelineBuilder&& other) noexcept = default;

  GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder& other) = delete;

  GraphicsPipelineBuilder& operator=(GraphicsPipelineBuilder&& other) noexcept = default;

  ~GraphicsPipelineBuilder() = default;

  Pipeline createPipeline(const Renderpass& renderpass, const PipelineLayout& pipelineLayout);

  static std::vector<Pipeline> createPipelines(
      std::span<const Renderpass> renderpasses, std::span<const GraphicsPipelineBuilder> builders,
      std::span<const PipelineLayout> pipelineLayouts);

  GraphicsPipelineBuilder& withShaderStageCreateInfo(
      lib::Buffer<VkPipelineShaderStageCreateInfo>&& shaderStages,
      std::optional<SpecializationData> specializationData = std::nullopt);

  GraphicsPipelineBuilder& withShaderStageCreateInfo(
      std::span<const VkPipelineShaderStageCreateInfo> shaderStages,
      std::optional<SpecializationData> specializationData = std::nullopt);

  GraphicsPipelineBuilder& withVertexInputStateCreateInfo(
      lib::Buffer<VkVertexInputBindingDescription>&& vertexInputBindingDescriptions,
      lib::Buffer<VkVertexInputAttributeDescription>&& vertexInputAttributeDescriptions);

  GraphicsPipelineBuilder& withVertexInputStateCreateInfo(
      std::span<const VkVertexInputBindingDescription> vertexInputBindingDescriptions,
      std::span<const VkVertexInputAttributeDescription> vertexInputAttributeDescriptions);

  GraphicsPipelineBuilder& withInputAssemblyStateCreateInfo(
      VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE,
      VkPipelineInputAssemblyStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withTessellationStateCreateInfo(
      uint32_t patchControlPoints, VkPipelineTessellationStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withViewportStateCreateInfo(
      VkPipelineViewportStateCreateFlags flags = 0, std::span<const VkViewport> viewports = {},
      std::span<const VkRect2D> scissors = {});

  GraphicsPipelineBuilder& withRasterizationStateCreateInfo(
      VkPolygonMode polygonMode, VkCullModeFlags cullMode,
      std::optional<std::pair<float, float>> depthBiasConstantSlopeFactors = std::nullopt,
      float depthBiasClamp = 0.0f, float lineWidth = 1.0f);

  GraphicsPipelineBuilder& withMultisampleStateCreateInfo(
      VkSampleCountFlagBits numMsaaSamples, std::optional<float> minSampleShading = std::nullopt,
      std::span<const VkSampleMask> sampleMasks = {},
      VkPipelineMultisampleStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withColorBlendStateCreateInfo(
      lib::Buffer<VkPipelineColorBlendAttachmentState>&& colorBlendAttachments,
      std::array<float, 4> blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
      std::optional<VkLogicOp> logicOp = std::nullopt,
      VkPipelineColorBlendStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withColorBlendStateCreateInfo(
      std::span<const VkPipelineColorBlendAttachmentState> colorBlendAttachments,
      std::array<float, 4> blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
      std::optional<VkLogicOp> logicOp = std::nullopt,
      VkPipelineColorBlendStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withDepthStencilStateCreateInfo(
      std::optional<VkCompareOp> depthCompareOp,
      std::optional<std::pair<float, float>> depthBounds = std::nullopt,
      std::optional<std::pair<VkStencilOpState, VkStencilOpState>> frontBack = std::nullopt,
      VkPipelineDepthStencilStateCreateFlags flags = 0);

  GraphicsPipelineBuilder& withPushConstantShaderStages(VkShaderStageFlags shaderStageFlags);

  GraphicsPipelineBuilder& withSubpass(uint32_t subpassIndex);

  GraphicsPipelineBuilder& withFragmentShadingRateStateCreateInfo(
      VkExtent2D fragmentSize, VkFragmentShadingRateCombinerOpKHR combinerOp1,
      VkFragmentShadingRateCombinerOpKHR combinerOp2);

private:
  SpecializationData _specializationData;
  lib::Buffer<VkPipelineShaderStageCreateInfo> _shaderStages;
  VkShaderStageFlags _shaderStageFlags = 0;

  lib::Buffer<VkVertexInputBindingDescription> _vertexBindingDescriptions;
  lib::Buffer<VkVertexInputAttributeDescription> _vertexAttributeDescriptions;
  VkPipelineVertexInputStateCreateInfo _vertexInputState = {
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo _inputAssemblyState = {
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

  VkPipelineTessellationStateCreateInfo _tessellationState = {
    VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

  lib::Buffer<VkViewport> _viewports;
  lib::Buffer<VkRect2D> _scissors;
  VkPipelineViewportStateCreateInfo _viewportState = {
    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

  VkPipelineRasterizationStateCreateInfo _rasterizationState = {
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

  lib::Buffer<VkSampleMask> _sampleMasks;
  VkPipelineMultisampleStateCreateInfo _multisampleState = {
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

  lib::Buffer<VkPipelineColorBlendAttachmentState> _colorBlendAttachments;
  VkPipelineColorBlendStateCreateInfo _colorBlendState = {
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};

  VkPipelineDepthStencilStateCreateInfo _depthStencilState = {
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

  lib::Buffer<VkDynamicState> _dynamicStates;
  VkPipelineDynamicStateCreateInfo _dynamicState = {
    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};

  std::optional<VkPipelineFragmentShadingRateStateCreateInfoKHR> _fragmentShadingRateState;

  uint32_t _subpassIndex = 0;

  void* _pNext = nullptr;
};
