#include "pipeline_manager.h"

#include <filesystem>
#include <numeric>
#include <string_view>
#include <vulkan/vulkan.h>

#include "lib/buffer/buffer.h"
#include "vulkan/resource_manager/util.h"
#include "vulkan/wrapper/pipeline/compute_pipeline_builder.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/util/vertex_input_description_builder.h"

PipelineManager::PipelineManager(const FileLoader& fileLoader) : _fileLoader(fileLoader) {}

std::unique_ptr<PipelineManager> PipelineManager::create(const FileLoader& fileLoader) {
  return std::unique_ptr<PipelineManager>(new PipelineManager(fileLoader));
}

const Shader& PipelineManager::addShader(
    const LogicalDevice& logicalDevice, std::string_view shaderFile,
    VkShaderStageFlagBits shaderStages) {
  auto [it, inserted] = _shaders.try_emplace(shaderFile);
  if (!inserted) {
    return it->second;
  }

  const lib::Buffer<std::byte> shaderData =
      _fileLoader.loadFileToBuffer((std::filesystem::path(SHADERS_PATH) / shaderFile).string());
  return it->second = Shader::create(logicalDevice, shaderData, shaderStages);
}

std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
PipelineManager::getOrCreateBindlessLayout(const LogicalDevice& logicalDevice) {
  static constexpr DescriptorSetType layoutType = DescriptorSetType::BINDLESS;
  if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
    return std::make_pair(it->second.first.getVkDescriptorSetLayout(), std::ref(it->second.second));
  }

  auto [layout, metadata] =
      DescriptorSetLayoutBuilder()
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 200, VK_SHADER_STAGE_ALL_GRAPHICS,
                      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                          | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
          .addBinding(
              1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200, VK_SHADER_STAGE_ALL_GRAPHICS,
              VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                  | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
          .withFlags(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT)
          .buildWithMetadata(logicalDevice);
  const VkDescriptorSetLayout vkLayout = layout.getVkDescriptorSetLayout();
  auto emplaced =
      _descriptorSetLayouts.emplace(layoutType, std::make_pair(std::move(layout), metadata));
  return std::make_pair(vkLayout, std::ref(emplaced.first->second.second));
}

std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
PipelineManager::getOrCreateCameraLayout(const LogicalDevice& logicalDevice, bool multiview) {
  static constexpr DescriptorSetType layoutType = DescriptorSetType::CAMERA;
  if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
    return std::make_pair(it->second.first.getVkDescriptorSetLayout(), std::ref(it->second.second));
  }

  auto [layout, metadata] =
      DescriptorSetLayoutBuilder()
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, multiview ? 2u : 1u,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .buildWithMetadata(logicalDevice);

  const VkDescriptorSetLayout vkLayout = layout.getVkDescriptorSetLayout();
  auto emplaced =
      _descriptorSetLayouts.emplace(layoutType, std::make_pair(std::move(layout), metadata));
  return std::make_pair(vkLayout, std::ref(emplaced.first->second.second));
}

std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
PipelineManager::getOrCreateComputeLayout(const LogicalDevice& logicalDevice) {
  static constexpr DescriptorSetType layoutType = DescriptorSetType::COMPUTE;
  if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
    return std::make_pair(it->second.first.getVkDescriptorSetLayout(), std::ref(it->second.second));
  }

  auto [layout, metadata] =
      DescriptorSetLayoutBuilder()
          .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT)
          .buildWithMetadata(logicalDevice);
  const VkDescriptorSetLayout vkLayout = layout.getVkDescriptorSetLayout();
  auto emplaced =
      _descriptorSetLayouts.emplace(layoutType, std::make_pair(std::move(layout), metadata));
  return std::make_pair(vkLayout, std::ref(emplaced.first->second.second));
}

std::pair<PipelineLayout*, PipelineManager::PipelineLayoutMapIndex> PipelineManager::
    getOrCreatePipelineLayout(const PipelineLayoutKey& key, const LogicalDevice& logicalDevice) {
  auto [it, inserted] = _pipelineLayoutIndices.try_emplace(key);

  if (!inserted) {
    PipelineLayoutID& layoutID = _pipelineLayouts.getValue(it->second);
    layoutID.refCount++;
    return {&layoutID.layout, it->second};
  }

  const PipelineLayoutMapIndex index =
      getNextHandle(_pipelineLayouts.size(), _freePipelineLayoutIndices);
  it->second = index;
  _pipelineLayoutKeys.emplace(index, key);
  return {
    &_pipelineLayouts
         .insertUnsafe(
             index, PipelineLayoutID{PipelineLayout::create(logicalDevice, key.descriptorSetLayouts,
                                                            key.pushConstants, key.createFlags),
                                     1})
         .layout,
    index};
}

bool PipelineManager::removePipelineLayout(PipelineLayoutMapIndex index) {
  if (!_pipelineLayouts.exists(index)) {
    return false;
  }

  PipelineLayoutID& layoutID = _pipelineLayouts.getValue(index);
  if (--layoutID.refCount > 0) {
    return true;
  }

  _pipelineLayouts.eraseUnsafe(index);
  _freePipelineLayoutIndices.push_back(index);

  auto it = _pipelineLayoutKeys.find(index);
  _pipelineLayoutIndices.erase(it->second);
  _pipelineLayoutKeys.erase(it);
  return true;
}

Pipeline* PipelineManager::getPipeline(PipelineHandle index) {
  if (!_pipelines.exists(*index)) {
    return nullptr;
  }

  return &_pipelines.getValue(*index).pipeline;
}

bool PipelineManager::removePipeline(PipelineHandle index) {
  if (!_pipelines.exists(*index)) {
    return false;
  }

  const PipelineLayoutMapIndex layoutIndex = _pipelines.getValue(*index).layoutIndex;
  _pipelines.eraseUnsafe(*index);
  _freePipelineIndices.push_back(index);

  return removePipelineLayout(layoutIndex);
}

namespace {

template <typename T>
constexpr VkPushConstantRange getPushConstantRange(
    VkShaderStageFlags shaderStages, uint32_t offset = 0) {
  return VkPushConstantRange{.stageFlags = shaderStages, .offset = offset, .size = sizeof(T)};
}

}  // namespace

PipelineHandle PipelineManager::createPBRProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex =
      addShader(logicalDevice, multiview ? "shader_pbr_multiview.vert.spv" : "shader_pbr.vert.spv",
                VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{
        {getOrCreateBindlessLayout(logicalDevice).first,
         getOrCreateCameraLayout(logicalDevice, multiview).first},
        {getPushConstantRange<PushConstantsModelDescriptorHandles32Bit>(shaderStageFlags)}
  },
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec2>()
      .addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec3>()
      .finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples())
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .withFragmentShadingRateStateCreateInfo(
                {1, 1}, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createPbrTesselationProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex =
      addShader(logicalDevice, "shader_pbr_tesselation.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& tesselationControl = addShader(
      logicalDevice, "shader_pbr_tesselation.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  const Shader& tesselationEvaluation = addShader(
      logicalDevice,
      multiview ? "shader_pbr_tesselation_multiview.tse.spv" : "shader_pbr_tesselation.tse.spv",
      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "shader_pbr_tesselation.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
      | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{
        {getOrCreateBindlessLayout(logicalDevice).first,
         getOrCreateCameraLayout(logicalDevice, multiview).first},
        {getPushConstantRange<PushConstantsModelDescriptorHandles32Bit>(shaderStageFlags)}
  },
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec2>()
      .addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec3>()
      .finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), tesselationControl.getVkPipelineStageCreateInfo(),
    tesselationEvaluation.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples())
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .withFragmentShadingRateStateCreateInfo(
                {1, 1}, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR)
            .withTessellationStateCreateInfo(3)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createBlinnPhongTesselationProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex =
      addShader(logicalDevice, "shader_blinn_phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& tesselationControl = addShader(
      logicalDevice, "shader_blinn_phong.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  const Shader& tesselationEvaluation = addShader(
      logicalDevice, "shader_blinn_phong.tse.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "shader_blinn_phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
      | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{
        {getOrCreateBindlessLayout(logicalDevice).first,
         getOrCreateCameraLayout(logicalDevice, multiview).first},
        {getPushConstantRange<PushConstantsModelDescriptorHandles32Bit>(shaderStageFlags)}
  },
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec2>()
      .addVertexAttributeDescription<glm::vec3>()
      .finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), tesselationControl.getVkPipelineStageCreateInfo(),
    tesselationEvaluation.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples())
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .withFragmentShadingRateStateCreateInfo(
                {1, 1}, VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR)
            .withTessellationStateCreateInfo(3)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createPbrEnvMappingProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex =
      addShader(logicalDevice, "pbr_env_mapping.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{
        {getOrCreateBindlessLayout(logicalDevice).first},
        {getPushConstantRange<PushConstantsModelDescriptorHandles32Bit>(shaderStageFlags)}},
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec2>()
      .addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec3>()
      .finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples(), 0.2f)
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createEnvMappingProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex =
      addShader(logicalDevice,
                multiview ? "env_mapping_phong_multiview.vert.spv" : "env_mapping_phong.vert.spv",
                VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "env_mapping_phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{
        {getOrCreateBindlessLayout(logicalDevice).first,
         getOrCreateCameraLayout(logicalDevice, multiview).first},
        {getPushConstantRange<PushConstantsModelDescriptorHandles32Bit>(shaderStageFlags)}
  },
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>()
      .addVertexAttributeDescription<glm::vec3>()
      .finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples(), 0.2f)
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createSkyboxProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex = addShader(logicalDevice, "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{{getOrCreateBindlessLayout(logicalDevice).first},
                        {getPushConstantRange<PushConstantsSkybox>(shaderStageFlags)}},
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>().finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT)
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples(), 0.2f)
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createShadowProgram(
    const Renderpass& renderpass, const AttachmentLayout& attachmentLayout) {
  const LogicalDevice& logicalDevice = renderpass.getLogicalDevice();
  const Shader& vertex = addShader(logicalDevice, "shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  const Shader& fragment =
      addShader(logicalDevice, "shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

  const VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{{}, {getPushConstantRange<PushConstantsShadow>(shaderStageFlags)}},
      logicalDevice);

  lib::Buffer<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
      attachmentLayout.getColorAttachmentsCount(),
      VkPipelineColorBlendAttachmentState{.colorWriteMask = VK_COLOR_COMPONENT_R_BIT});

  VertexInputDescriptionBuilder builder;
  builder.addVertexAttributeDescription<glm::vec3>().finishBinding(VK_VERTEX_INPUT_RATE_VERTEX);
  auto [bindingDescriptions, attributeDescriptions] = builder.getDescription();

  const VkPipelineShaderStageCreateInfo shaderStages[] = {
    vertex.getVkPipelineStageCreateInfo(), fragment.getVkPipelineStageCreateInfo()};

  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{
        GraphicsPipelineBuilder({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .withInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .withVertexInputStateCreateInfo(bindingDescriptions, attributeDescriptions)
            .withShaderStageCreateInfo(shaderStages)
            .withPushConstantShaderStages(shaderStageFlags)
            .withViewportStateCreateInfo()
            .withRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, std::pair{0.7f, 2.0f})
            .withMultisampleStateCreateInfo(attachmentLayout.getNumMsaaSamples())
            .withColorBlendStateCreateInfo(std::move(colorBlendAttachments))
            .withDepthStencilStateCreateInfo(VK_COMPARE_OP_LESS_OR_EQUAL)
            .createPipeline(renderpass, *pipelineLayout),
        pipelineLayoutIndex});
  return pipelineIndex;
}

PipelineHandle PipelineManager::createFragmentShadingRateProgram(
    const LogicalDevice& logicalDevice) {
  const Shader& compute =
      addShader(logicalDevice, "fov_fragment_shading_rate.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
  static constexpr VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  const auto [pipelineLayout, pipelineLayoutIndex] = getOrCreatePipelineLayout(
      PipelineLayoutKey{{getOrCreateComputeLayout(logicalDevice).first},
                        {getPushConstantRange<PushConstantFov>(shaderStageFlags)}},
      logicalDevice);
  const PipelineHandle pipelineIndex = getNextHandle(_pipelines.size(), _freePipelineIndices);
  _pipelines.insertUnsafe(
      *pipelineIndex,
      PipelineResource{ComputePipelineBuilder()
                           .withShaderStageCreateInfo(compute.getVkPipelineStageCreateInfo())
                           .createPipeline(*pipelineLayout),
                       pipelineLayoutIndex});
  return pipelineIndex;
}
