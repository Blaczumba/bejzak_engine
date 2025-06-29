#pragma once

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

class ShaderProgram {
protected:
	std::vector<DescriptorSetLayout> _descriptorSetLayouts;
	std::vector<Shader> _shaders;
	
	const LogicalDevice& _logicalDevice;
	std::optional<VkPushConstantRange> _pushConstants;

public:
	ShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, std::optional<VkPushConstantRange> pushConstantRange);
	std::vector<VkPipelineShaderStageCreateInfo> getVkPipelineShaderStageCreateInfos() const;

    std::span<const DescriptorSetLayout> getDescriptorSetLayouts() const;
	const std::optional<VkPushConstantRange>& getPushConstants() const;
};

class GraphicsShaderProgram : public ShaderProgram {
protected:
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;

public:
    template<typename VertexType>
    GraphicsShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, VertexType, std::optional<VkPushConstantRange> pushConstantRange = std::nullopt)
        : ShaderProgram(logicalDevice, std::move(shaders), std::move(descriptorSetLayouts), pushConstantRange) {
        static constexpr VkVertexInputBindingDescription bindingDescription = getBindingDescription<VertexType>();
        static constexpr auto attributeDescriptions = getAttributeDescriptions<VertexType>();
        _vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
    }
	const VkPipelineVertexInputStateCreateInfo& getVkPipelineVertexInputStateCreateInfo() const {
        return _vertexInputInfo;
	}
};

enum class ShaderProgramType {
    NORMAL,
    PBR,
    PBR_TESSELLATION,
    SKYBOX,
    SHADOW,
    SKYBOX_OFFSCREEN,
    PBR_OFFSCREEN
};

struct alignas(glm::vec4) PushConstantsPBR {
    uint32_t camera;
    uint32_t light;
    uint32_t diffuse;
    uint32_t normal;
    uint32_t metallicRoughness;
    uint32_t shadow;
    uint32_t padding[2];
    glm::mat4 model;
};

struct alignas(glm::vec4) PushConstantsShadow {
    glm::mat4 model;
};


class ShaderProgramFactory {
public:
    static std::unique_ptr<GraphicsShaderProgram> createShaderProgram(ShaderProgramType type, const LogicalDevice& logicalDevice) {
        switch (type) {
        case ShaderProgramType::NORMAL:
            return createNormalMappingProgram(logicalDevice);
            break;
        case ShaderProgramType::PBR:
            return createPBRProgram(logicalDevice);
            break;
        case ShaderProgramType::PBR_TESSELLATION:
            return createPBRTesselationProgram(logicalDevice);
            break;
        case ShaderProgramType::SKYBOX:
            return createSkyboxProgram(logicalDevice);
            break;
        case ShaderProgramType::SHADOW:
            return createShadowProgram(logicalDevice);
            break;
        case ShaderProgramType::SKYBOX_OFFSCREEN:
            return createSkyboxOffscreenProgram(logicalDevice);
            break;
        case ShaderProgramType::PBR_OFFSCREEN:
            return createPBROffscreenProgram(logicalDevice);
            break;
        default:
            throw std::invalid_argument("Unsupported ShaderProgramType");
        }
    }

private:
    static std::unique_ptr<GraphicsShaderProgram> createNormalMappingProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createPBRProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createPBRTesselationProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createSkyboxProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createShadowProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createSkyboxOffscreenProgram(const LogicalDevice& logicalDevice);
    static std::unique_ptr<GraphicsShaderProgram> createPBROffscreenProgram(const LogicalDevice& logicalDevice);
};