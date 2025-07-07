#include "shader_program.h"

#include "descriptor_set/descriptor_set_layout.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "primitives/vk_primitives.h"
#include "primitives/primitives.h"

#include <algorithm>
#include <iterator>
#include <memory>

ShaderProgram::ShaderProgram(const ShaderProgramManager& shaderProgramManager, std::vector<std::string_view>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange)
    : _shaderProgramManager(shaderProgramManager), _shaders(std::move(shaders)), _descriptorSetLayouts(std::move(descriptorSetLayouts)), _pushConstants(pushConstantRange.cbegin(), pushConstantRange.cend()) {}

std::vector<VkDescriptorSetLayout> ShaderProgram::getVkDescriptorSetLayouts() const {
    std::vector<VkDescriptorSetLayout> layouts;
    std::transform(_descriptorSetLayouts.cbegin(), _descriptorSetLayouts.cend(), std::back_inserter(layouts), [this](DescriptorSetType layoutType) { return _shaderProgramManager.getVkDescriptorSetLayout(layoutType); });
    return layouts;
}

std::span<const DescriptorSetType> ShaderProgram::getDescriptorSetLayouts() const {
    return _descriptorSetLayouts;
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderProgram::getShaders() const {
    std::vector<VkPipelineShaderStageCreateInfo> shaders;
    std::transform(_shaders.cbegin(), _shaders.cend(), std::back_inserter(shaders), [this](std::string_view shaderPath) { return _shaderProgramManager.getShader(shaderPath)->getVkPipelineStageCreateInfo(); });
    return shaders;
}

std::span<const VkPushConstantRange> ShaderProgram::getPushConstants() const {
    return _pushConstants;
}

GraphicsShaderProgram::GraphicsShaderProgram(const ShaderProgramManager& shaderProgramManager, const VkPipelineVertexInputStateCreateInfo& vertexInputInfo, std::vector<std::string_view>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange)
    : ShaderProgram(shaderProgramManager, std::move(shaders), std::move(descriptorSetLayouts), pushConstantRange), _vertexInputInfo(vertexInputInfo) {
}

const VkPipelineVertexInputStateCreateInfo& GraphicsShaderProgram::getVkPipelineVertexInputStateCreateInfo() const {
    return _vertexInputInfo;
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createPBRProgram() {
    static constexpr std::string_view vertexShaderPath = "shader_pbr.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "shader_pbr.frag.spv";
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));
    _shaders.emplace(vertexShaderPath, std::move(vertexShader));
    _shaders.emplace(fragmentShaderPath, std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsPBR>(VK_SHADER_STAGE_ALL);

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());
    ASSIGN_OR_RETURN(DescriptorSetType cameraLayout, getOrCreateCameraLayout());

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexPTNT>();

    return std::make_unique<GraphicsShaderProgram>(*this, vertexInputInfo, std::vector{ vertexShaderPath, fragmentShaderPath }, std::vector{bindlessLayout, cameraLayout}, pushConstants.getVkPushConstantRange());
}

ShaderProgramManager::ShaderProgramManager(const LogicalDevice& logicalDevice) : _logicalDevice(logicalDevice) { }

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createSkyboxProgram() {
    static constexpr std::string_view vertexShaderPath = "skybox.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "skybox.frag.spv";
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));
    _shaders.emplace(vertexShaderPath, std::move(vertexShader));
    _shaders.emplace(fragmentShaderPath, std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsSkybox>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexP>();

    return std::make_unique<GraphicsShaderProgram>(*this, vertexInputInfo, std::vector{ vertexShaderPath, fragmentShaderPath }, std::vector{ bindlessLayout }, pushConstants.getVkPushConstantRange());
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createShadowProgram() {
    static constexpr std::string_view vertexShaderPath = "shadow.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "shadow.frag.spv";
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));
    _shaders.emplace(vertexShaderPath, std::move(vertexShader));
    _shaders.emplace(fragmentShaderPath, std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsShadow>(VK_SHADER_STAGE_VERTEX_BIT);

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexP>();

    return std::make_unique<GraphicsShaderProgram>(*this, vertexInputInfo, std::vector{ vertexShaderPath, fragmentShaderPath }, std::vector<DescriptorSetType>{}, pushConstants.getVkPushConstantRange());
}

const VkDescriptorSetLayout ShaderProgramManager::getVkDescriptorSetLayout(DescriptorSetType type) const {
    return _descriptorSetLayouts.find(type)->second.getVkDescriptorSetLayout();
}

const DescriptorSetLayout* ShaderProgramManager::getDescriptorSetLayout(DescriptorSetType type) const {
    return &_descriptorSetLayouts.find(type)->second;
}

const Shader* ShaderProgramManager::getShader(std::string_view shaderPath) const {
    return &_shaders.find(shaderPath)->second;
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateBindlessLayout() {
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
    static constexpr VkDescriptorBindingFlags flags{ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };
    static constexpr VkDescriptorBindingFlags bindingFlags[] = { flags, flags };
    ASSIGN_OR_RETURN(DescriptorSetLayout layout, DescriptorSetLayout::create(_logicalDevice, bindings, bindingFlags, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
    _descriptorSetLayouts.emplace(layoutType, std::move(layout));
    return layoutType;
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateCameraLayout() {
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
    ASSIGN_OR_RETURN(DescriptorSetLayout layout, DescriptorSetLayout::create(_logicalDevice, bindings));
    _descriptorSetLayouts.emplace(layoutType, std::move(layout));
    return layoutType;
}

const LogicalDevice& ShaderProgramManager::getLogicalDevice() const {
    return _logicalDevice;
}