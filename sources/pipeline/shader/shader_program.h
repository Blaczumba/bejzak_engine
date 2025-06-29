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

class ShaderProgram {
protected:
	std::vector<DescriptorSetLayout> _descriptorSetLayouts;
	std::vector<Shader> _shaders;
	
	const LogicalDevice& _logicalDevice;
	std::vector<VkPushConstantRange> _pushConstants;

public:
	ShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange);
	std::vector<VkPipelineShaderStageCreateInfo> getVkPipelineShaderStageCreateInfos() const;

    std::span<const DescriptorSetLayout> getDescriptorSetLayouts() const;
	std::span<const VkPushConstantRange> getPushConstants() const;
};

class GraphicsShaderProgram : public ShaderProgram {
private:
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;

    GraphicsShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, const VkPipelineVertexInputStateCreateInfo& vertexInputInfo, std::span<const VkPushConstantRange> pushConstantRange)
        : ShaderProgram(logicalDevice, std::move(shaders), std::move(descriptorSetLayouts), pushConstantRange), _vertexInputInfo(vertexInputInfo) {
    }

public:
    template<typename VertexType>
    static std::unique_ptr<GraphicsShaderProgram> create(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange = {}) {
        static constexpr VkVertexInputBindingDescription bindingDescription = getBindingDescription<VertexType>();
        static constexpr auto attributeDescriptions = getAttributeDescriptions<VertexType>();
        const VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
        return std::unique_ptr<GraphicsShaderProgram>(new GraphicsShaderProgram(logicalDevice, std::move(shaders), std::move(descriptorSetLayouts), vertexInputInfo, pushConstantRange));
    }


	const VkPipelineVertexInputStateCreateInfo& getVkPipelineVertexInputStateCreateInfo() const {
        return _vertexInputInfo;
	}
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
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createNormalMappingProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createPBRProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createPBRTesselationProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createSkyboxProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createShadowProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createSkyboxOffscreenProgram(const LogicalDevice& logicalDevice);
    static ErrorOr<std::unique_ptr<GraphicsShaderProgram>> createPBROffscreenProgram(const LogicalDevice& logicalDevice);
};