#pragma once

#include "status/status.h"
#include "descriptor_set/descriptor_set_layout.h"
#include "memory_objects/uniform_buffer/push_constants.h"
#include "primitives/vk_primitives.h"
#include "shader.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <span>
#include <vector>

class LogicalDevice;
class PushConstants;
class DescriptorSetLayout;
class GraphicsShaderProgram;

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

    const VkDescriptorSetLayout getVkDescriptorSetLayout(DescriptorSetType type) const;

    const Shader* getShader(std::string_view shaderPath) const;

    const DescriptorSetLayout* getDescriptorSetLayout(DescriptorSetType type) const;

    ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createPBRProgram();

    ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createSkyboxProgram();

    ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createShadowProgram();

    const LogicalDevice& getLogicalDevice() const;

private:
    ErrorOr<DescriptorSetType> getOrCreateBindlessLayout();

    ErrorOr<DescriptorSetType> getOrCreateCameraLayout();
};

class ShaderProgram {
protected:
	std::vector<DescriptorSetType> _descriptorSetLayouts;   // TODO: std::inplace_vector<DescriptorSetType, 4>
	std::vector<std::string_view> _shaders;
	std::vector<VkPushConstantRange> _pushConstants;

	const ShaderProgramManager& _shaderProgramManager;

public:
	ShaderProgram(const ShaderProgramManager& shaderProgramManager, std::vector<std::string_view>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange);

    std::vector<VkPipelineShaderStageCreateInfo> getShaders() const;

    std::vector<VkDescriptorSetLayout> getVkDescriptorSetLayouts() const;

    std::span<const DescriptorSetType> getDescriptorSetLayouts() const;

	std::span<const VkPushConstantRange> getPushConstants() const;
};

class GraphicsShaderProgram : public ShaderProgram {
private:
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;

    GraphicsShaderProgram(const ShaderProgramManager& shaderProgramManager, std::vector<std::string_view>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, const VkPipelineVertexInputStateCreateInfo& vertexInputInfo, std::span<const VkPushConstantRange> pushConstantRange)
        : ShaderProgram(shaderProgramManager, std::move(shaders), std::move(descriptorSetLayouts), pushConstantRange), _vertexInputInfo(vertexInputInfo) {
    }

public:
    template<typename VertexType>
    static std::unique_ptr<GraphicsShaderProgram> create(const ShaderProgramManager& shaderProgramManager, std::vector<std::string_view>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange = {}) {
        static constexpr VkVertexInputBindingDescription bindingDescription = getBindingDescription<VertexType>();
        static constexpr auto attributeDescriptions = getAttributeDescriptions<VertexType>();
        const VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
        return std::unique_ptr<GraphicsShaderProgram>(new GraphicsShaderProgram(shaderProgramManager, std::move(shaders), std::move(descriptorSetLayouts), vertexInputInfo, pushConstantRange));
    }


	const VkPipelineVertexInputStateCreateInfo& getVkPipelineVertexInputStateCreateInfo() const {
        return _vertexInputInfo;
	}
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
