#pragma once

#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include "common/status/status.h"
#include "common/util/primitives.h"
#include "input_description.h"
#include "lib/buffer/buffer.h"
#include "shader.h"
#include "vulkan_wrapper/descriptor_set/descriptor_set_layout.h"

class LogicalDevice;
class DescriptorSetLayout;
class ShaderProgram;

enum class DescriptorSetType : uint8_t {
  BINDLESS,
  CAMERA
};

class ShaderProgramManager {
  std::unordered_map<std::string_view, Shader> _shaders;
  std::unordered_map<DescriptorSetType, DescriptorSetLayout> _descriptorSetLayouts;

public:
  const Shader* getShader(std::string_view shaderPath) const;

  const VkDescriptorSetLayout getVkDescriptorSetLayout(DescriptorSetType type) const;

  ErrorOr<ShaderProgram> createPBRProgram(const LogicalDevice& _logicalDevice);

  ErrorOr<ShaderProgram> createSkyboxProgram(const LogicalDevice& _logicalDevice);

  ErrorOr<ShaderProgram> createShadowProgram(const LogicalDevice& _logicalDevice);

private:
  Status addShader(const LogicalDevice& logicalDevice, std::string_view shaderFile,
                   VkShaderStageFlagBits shaderStages);

  ErrorOr<DescriptorSetType> getOrCreateBindlessLayout(const LogicalDevice& logicalDevice);

  ErrorOr<DescriptorSetType> getOrCreateCameraLayout(const LogicalDevice& logicalDevice);

  template <typename T>
  static constexpr VkPushConstantRange getPushConstantRange(
      VkShaderStageFlags shaderStages, uint32_t offset = 0) {
    return VkPushConstantRange{.stageFlags = shaderStages, .offset = offset, .size = sizeof(T)};
  }

  template <typename VertexType>
  static VkPipelineVertexInputStateCreateInfo getVkPipelineVertexInputStateCreateInfo() {
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
};

class ShaderProgram {
protected:
  std::vector<DescriptorSetType> _descriptorSetLayouts;  // TODO: std::inplace_vector
  std::vector<std::string_view> _shaders;
  std::vector<VkPushConstantRange> _pushConstants;

  std::optional<VkPipelineVertexInputStateCreateInfo> _vertexInputInfo;

  const ShaderProgramManager* _shaderProgramManager;

public:
  ShaderProgram() = default;

  ShaderProgram(const ShaderProgramManager& shaderProgramManager,
                std::initializer_list<std::string_view> shaders,
                std::initializer_list<DescriptorSetType> descriptorSetLayouts,
                std::span<const VkPushConstantRange> pushConstantRange,
                std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputInfo = std::nullopt);

  ShaderProgram(ShaderProgram&& shaderProgram) noexcept;

  ShaderProgram& operator=(ShaderProgram&& shaderProgram) noexcept;

  virtual ~ShaderProgram() = default;

  lib::Buffer<VkPipelineShaderStageCreateInfo> getVkPipelineShaderStageCreateInfos() const;

  lib::Buffer<VkDescriptorSetLayout> getVkDescriptorSetLayouts() const;

  std::span<const DescriptorSetType> getDescriptorSetLayouts() const;

  std::span<const VkPushConstantRange> getPushConstants() const;

  const std::optional<VkPipelineVertexInputStateCreateInfo>&
  getVkPipelineVertexInputStateCreateInfo() const;
};

struct PushConstantsPBR {
  glm::mat4 model;
  uint32_t light;
  uint32_t diffuse;
  uint32_t normal;
  uint32_t metallicRoughness;
  uint32_t shadow;
  uint32_t padding[3];
};

struct PushConstantsShadow {
  glm::mat4 model;
  glm::mat4 lightProjView;
};

struct PushConstantsSkybox {
  glm::mat4 proj;
  glm::mat3x4 view;
  uint32_t skyboxHandle;
};
