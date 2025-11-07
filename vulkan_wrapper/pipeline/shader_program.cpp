#include "shader_program.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>

#include "common/util/primitives.h"
#include "vulkan_wrapper/descriptor_set/descriptor_set_layout.h"
#include "vulkan_wrapper/logical_device/logical_device.h"
#include "vulkan_wrapper/pipeline/input_description.h"

ShaderProgram::ShaderProgram(
    const ShaderProgramManager& shaderProgramManager,
    std::initializer_list<std::string_view> shaders,
    std::initializer_list<DescriptorSetType> descriptorSetLayouts,
    std::span<const VkPushConstantRange> pushConstantRange,
    std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputInfo)
  : _shaderProgramManager(&shaderProgramManager), _shaders(shaders),
    _descriptorSetLayouts(descriptorSetLayouts),
    _pushConstants(std::cbegin(pushConstantRange), std::cend(pushConstantRange)),
    _vertexInputInfo(vertexInputInfo) {}

ShaderProgram::ShaderProgram(ShaderProgram&& shaderProgram) noexcept
  : _descriptorSetLayouts(std::move(shaderProgram._descriptorSetLayouts)),
    _shaders(std::move(shaderProgram._shaders)),
    _pushConstants(std::move(shaderProgram._pushConstants)),
    _vertexInputInfo(shaderProgram._vertexInputInfo),
    _shaderProgramManager(std::exchange(shaderProgram._shaderProgramManager, nullptr)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& shaderProgram) noexcept {
  if (this == &shaderProgram) {
    return *this;
  }
  _descriptorSetLayouts = std::move(shaderProgram._descriptorSetLayouts);
  _shaders = std::move(shaderProgram._shaders);
  _pushConstants = std::move(shaderProgram._pushConstants);
  _vertexInputInfo = shaderProgram._vertexInputInfo;
  _shaderProgramManager = std::exchange(shaderProgram._shaderProgramManager, nullptr);
  return *this;
}

lib::Buffer<VkDescriptorSetLayout> ShaderProgram::getVkDescriptorSetLayouts() const {
  lib::Buffer<VkDescriptorSetLayout> layouts(_descriptorSetLayouts.size());
  std::transform(_descriptorSetLayouts.cbegin(), _descriptorSetLayouts.cend(), layouts.begin(),
                 [this](DescriptorSetType layoutType) {
                   return _shaderProgramManager->getVkDescriptorSetLayout(layoutType);
                 });
  return layouts;
}

lib::Buffer<VkPipelineShaderStageCreateInfo>
ShaderProgram::getVkPipelineShaderStageCreateInfos() const {
  lib::Buffer<VkPipelineShaderStageCreateInfo> shaders(_shaders.size());
  std::transform(
      _shaders.cbegin(), _shaders.cend(), shaders.begin(), [this](std::string_view shaderPath) {
        return _shaderProgramManager->getShader(shaderPath)->getVkPipelineStageCreateInfo();
      });
  return shaders;
}

std::span<const DescriptorSetType> ShaderProgram::getDescriptorSetLayouts() const {
  return _descriptorSetLayouts;
}

std::span<const VkPushConstantRange> ShaderProgram::getPushConstants() const {
  return _pushConstants;
}

const std::optional<VkPipelineVertexInputStateCreateInfo>&
ShaderProgram::getVkPipelineVertexInputStateCreateInfo() const {
  return _vertexInputInfo;
}

ShaderProgramManager::ShaderProgramManager(const std::shared_ptr<FileLoader>& fileLoader)
  : _fileLoader(fileLoader) {}

namespace {

template <typename T>
constexpr VkPushConstantRange getPushConstantRange(
    VkShaderStageFlags shaderStages, uint32_t offset = 0) {
  return VkPushConstantRange{.stageFlags = shaderStages, .offset = offset, .size = sizeof(T)};
}

template <typename VertexType>
VkPipelineVertexInputStateCreateInfo getVkPipelineVertexInputStateCreateInfo() {
  static constexpr VkVertexInputBindingDescription bindingDescription =
      getBindingDescription<VertexType>();
  static constexpr auto attributeDescriptions = getAttributeDescriptions<VertexType>();
  return VkPipelineVertexInputStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data()};
}

}  // namespace

ErrorOr<ShaderProgram> ShaderProgramManager::createPbrEnvMappingProgram(
    const LogicalDevice& logicalDevice) {
  static constexpr std::string_view vertexShaderPath = "pbr_env_mapping.vert.spv";
  static constexpr std::string_view fragmentShaderPath = "shader_pbr.frag.spv";
  RETURN_IF_ERROR(addShader(logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  RETURN_IF_ERROR(addShader(logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

  static constexpr VkPushConstantRange pushConstantRanges[] = {
    getPushConstantRange<PushConstantsPBR>(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

  ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout(logicalDevice));

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVkPipelineVertexInputStateCreateInfo<VertexPTNT>();

  return ShaderProgram(*this, {vertexShaderPath, fragmentShaderPath}, {bindlessLayout},
                       pushConstantRanges, vertexInputInfo);
}

ErrorOr<ShaderProgram> ShaderProgramManager::createPhongWithEnvMappingProgram(
    const LogicalDevice& logicalDevice) {
  static constexpr std::string_view vertexShaderPath = "env_mapping_phong.vert.spv";
  static constexpr std::string_view fragmentShaderPath = "env_mapping_phong.frag.spv";
  RETURN_IF_ERROR(addShader(logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  RETURN_IF_ERROR(addShader(logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

  static constexpr VkPushConstantRange pushConstantRanges[] = {
    getPushConstantRange<PushConstantsPhongEnv>(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

  ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout(logicalDevice));

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVkPipelineVertexInputStateCreateInfo<VertexPN>();

  return ShaderProgram(*this, {vertexShaderPath, fragmentShaderPath}, {bindlessLayout},
                       pushConstantRanges, vertexInputInfo);
}

ErrorOr<ShaderProgram> ShaderProgramManager::createPBRProgram(const LogicalDevice& logicalDevice) {
  static constexpr std::string_view vertexShaderPath = "shader_pbr.vert.spv";
  static constexpr std::string_view fragmentShaderPath = "shader_pbr.frag.spv";
  RETURN_IF_ERROR(addShader(logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  RETURN_IF_ERROR(addShader(logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

  static constexpr VkPushConstantRange pushConstantRanges[] = {
    getPushConstantRange<PushConstantsPBR>(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

  ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout(logicalDevice));
  ASSIGN_OR_RETURN(DescriptorSetType cameraLayout, getOrCreateCameraLayout(logicalDevice));

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVkPipelineVertexInputStateCreateInfo<VertexPTNT>();

  return ShaderProgram(*this, {vertexShaderPath, fragmentShaderPath},
                       {bindlessLayout, cameraLayout}, pushConstantRanges, vertexInputInfo);
}

ErrorOr<ShaderProgram> ShaderProgramManager::createSkyboxProgram(
    const LogicalDevice& logicalDevice) {
  static constexpr std::string_view vertexShaderPath = "skybox.vert.spv";
  static constexpr std::string_view fragmentShaderPath = "skybox.frag.spv";
  RETURN_IF_ERROR(addShader(logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  RETURN_IF_ERROR(addShader(logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

  ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout(logicalDevice));

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVkPipelineVertexInputStateCreateInfo<VertexP>();

  static constexpr VkPushConstantRange pushConstantRanges[] = {
    getPushConstantRange<PushConstantsSkybox>(
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

  return ShaderProgram(*this, {vertexShaderPath, fragmentShaderPath}, {bindlessLayout},
                       pushConstantRanges, vertexInputInfo);
}

ErrorOr<ShaderProgram> ShaderProgramManager::createShadowProgram(
    const LogicalDevice& logicalDevice) {
  static constexpr std::string_view vertexShaderPath = "shadow.vert.spv";
  static constexpr std::string_view fragmentShaderPath = "shadow.frag.spv";
  RETURN_IF_ERROR(addShader(logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
  RETURN_IF_ERROR(addShader(logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

  static constexpr VkPushConstantRange pushConstantRanges[] = {
    getPushConstantRange<PushConstantsShadow>(VK_SHADER_STAGE_VERTEX_BIT)};

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
      getVkPipelineVertexInputStateCreateInfo<VertexP>();

  return ShaderProgram(
      *this, {vertexShaderPath, fragmentShaderPath}, {}, pushConstantRanges, vertexInputInfo);
}

const VkDescriptorSetLayout ShaderProgramManager::getVkDescriptorSetLayout(
    DescriptorSetType type) const {
  if (auto it = _descriptorSetLayouts.find(type); it != _descriptorSetLayouts.cend()) {
    return it->second.getVkDescriptorSetLayout();
  }
  return VK_NULL_HANDLE;
}

const Shader* ShaderProgramManager::getShader(std::string_view shaderPath) const {
  if (auto it = _shaders.find(shaderPath); it != _shaders.cend()) {
    return &it->second;
  }
  return nullptr;
}

Status ShaderProgramManager::addShader(
    const LogicalDevice& logicalDevice, std::string_view shaderFile,
    VkShaderStageFlagBits shaderStages) {
  if (_shaders.contains(shaderFile)) {
    return StatusOk();
  }

  ASSIGN_OR_RETURN(
      const lib::Buffer<std::byte> shaderData,
      _fileLoader->loadFileToBuffer((std::filesystem::path(SHADERS_PATH) / shaderFile).string()));
  ASSIGN_OR_RETURN(Shader shader, Shader::create(logicalDevice, shaderData, shaderStages));
  _shaders.emplace(shaderFile, std::move(shader));
  return StatusOk();
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateBindlessLayout(
    const LogicalDevice& logicalDevice) {
  static constexpr DescriptorSetType layoutType = DescriptorSetType::BINDLESS;
  if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
    return it->first;
  }

  static constexpr VkDescriptorSetLayoutBinding bindings[] = {
    {
     .binding = 0,
     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     .descriptorCount = 200,
     .stageFlags = VK_SHADER_STAGE_ALL,
     },
    {
     .binding = 1,
     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
     .descriptorCount = 200,
     .stageFlags = VK_SHADER_STAGE_ALL,
     }
  };

  static constexpr VkDescriptorBindingFlags flags{
    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT};
  static constexpr VkDescriptorBindingFlags bindingFlags[] = {flags, flags};

  ASSIGN_OR_RETURN(
      DescriptorSetLayout layout,
      DescriptorSetLayout::create(logicalDevice, bindings, bindingFlags,
                                  VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
  _descriptorSetLayouts.emplace(layoutType, std::move(layout));
  return layoutType;
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateCameraLayout(
    const LogicalDevice& logicalDevice) {
  static constexpr DescriptorSetType layoutType = DescriptorSetType::CAMERA;
  if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
    return it->first;
  }
  static constexpr VkDescriptorSetLayoutBinding bindings[] = {
    {
     .binding = 0,
     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
     .descriptorCount = 1,
     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
     },
  };
  ASSIGN_OR_RETURN(
      DescriptorSetLayout layout, DescriptorSetLayout::create(logicalDevice, bindings));
  _descriptorSetLayouts.emplace(layoutType, std::move(layout));
  return layoutType;
}
