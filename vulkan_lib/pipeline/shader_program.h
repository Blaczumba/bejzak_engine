#pragma once

#include "vulkan_lib/descriptor_set/descriptor_set_layout.h"
#include "input_description.h"
#include "vulkan_lib/lib/buffer/buffer.h"
#include "vulkan_lib/primitives/primitives.h"
#include "shader.h"
#include "vulkan_lib/status/status.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

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

    const LogicalDevice& _logicalDevice;

public:
    ShaderProgramManager(const LogicalDevice& logicalDevice);

    const Shader* getShader(std::string_view shaderPath) const;

    const VkDescriptorSetLayout getVkDescriptorSetLayout(DescriptorSetType type) const;

    ErrorOr<std::unique_ptr<ShaderProgram>> createPBRProgram();

    ErrorOr<std::unique_ptr<ShaderProgram>> createSkyboxProgram();

    ErrorOr<std::unique_ptr<ShaderProgram>> createShadowProgram();

    const LogicalDevice& getLogicalDevice() const;

private:
    Status addShader(std::string_view shaderFile, VkShaderStageFlagBits shaderStages);

    ErrorOr<DescriptorSetType> getOrCreateBindlessLayout();

    ErrorOr<DescriptorSetType> getOrCreateCameraLayout();

    template<typename T>
    static constexpr VkPushConstantRange getPushConstantRange(VkShaderStageFlags shaderStages, uint32_t offset = 0) {
        return VkPushConstantRange{
            .stageFlags = shaderStages,
            .offset = offset,
            .size = sizeof(T)
        };
    }

    template<typename VertexType>
    static VkPipelineVertexInputStateCreateInfo getVkPipelineVertexInputStateCreateInfo() {
        static constexpr VkVertexInputBindingDescription bindingDescription = getBindingDescription<VertexType>();
        static constexpr auto attributeDescriptions = getAttributeDescriptions<VertexType>();
        return VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
    }
};

class ShaderProgram {
protected:
	std::vector<DescriptorSetType> _descriptorSetLayouts;   // TODO: std::inplace_vector<DescriptorSetType, 4>
	std::vector<std::string_view> _shaders;
	std::vector<VkPushConstantRange> _pushConstants;

    std::optional<VkPipelineVertexInputStateCreateInfo> _vertexInputInfo;

	const ShaderProgramManager& _shaderProgramManager;

public:
	ShaderProgram(const ShaderProgramManager& shaderProgramManager, std::initializer_list<std::string_view> shaders, std::initializer_list<DescriptorSetType> descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange, std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputInfo = std::nullopt);

    lib::Buffer<VkPipelineShaderStageCreateInfo> getVkPipelineShaderStageCreateInfos() const;

    lib::Buffer<VkDescriptorSetLayout> getVkDescriptorSetLayouts() const;

    std::span<const DescriptorSetType> getDescriptorSetLayouts() const;

	std::span<const VkPushConstantRange> getPushConstants() const;

    const std::optional<VkPipelineVertexInputStateCreateInfo>& getVkPipelineVertexInputStateCreateInfo() const;
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
