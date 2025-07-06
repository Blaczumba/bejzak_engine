#include "shader_program.h"

#include "descriptor_set/descriptor_set_layout.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "primitives/vk_primitives.h"
#include "primitives/primitives.h"

#include <algorithm>
#include <iterator>
#include <memory>

ShaderProgram::ShaderProgram(const ShaderProgramManager& shaderProgramManager, std::vector<Shader>&& shaders, std::vector<DescriptorSetType>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange)
    : _shaderProgramManager(shaderProgramManager), _shaders(std::move(shaders)), _descriptorSetLayouts(std::move(descriptorSetLayouts)), _pushConstants(pushConstantRange.cbegin(), pushConstantRange.cend()) {}

std::vector<VkDescriptorSetLayout> ShaderProgram::getVkDescriptorSetLayouts() const {
    std::vector<VkDescriptorSetLayout> layouts;
    std::transform(_descriptorSetLayouts.cbegin(), _descriptorSetLayouts.cend(), std::back_inserter(layouts), [this](DescriptorSetType layoutType) { return _shaderProgramManager.getVkDescriptorSetLayout(layoutType); });
    return layouts;
}

std::span<const DescriptorSetType> ShaderProgram::getDescriptorSetLayouts() const {
    return _descriptorSetLayouts;
}

std::span<const Shader> ShaderProgram::getShaders() const {
    return _shaders;
}

std::span<const VkPushConstantRange> ShaderProgram::getPushConstants() const {
    return _pushConstants;
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createPBRProgram() {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, SHADERS_PATH "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsPBR>(VK_SHADER_STAGE_ALL);

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());
    ASSIGN_OR_RETURN(DescriptorSetType cameraLayout, getOrCreateCameraLayout());

    return GraphicsShaderProgram::create<VertexPTNT>(*this, std::move(shaders), std::vector{bindlessLayout, cameraLayout}, pushConstants.getVkPushConstantRange());
}

ShaderProgramManager::ShaderProgramManager(const LogicalDevice& logicalDevice) : _logicalDevice(logicalDevice) { }

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createSkyboxProgram() {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, SHADERS_PATH "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, SHADERS_PATH "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsSkybox>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());

    return GraphicsShaderProgram::create<VertexP>(*this, std::move(shaders), std::vector{ bindlessLayout }, pushConstants.getVkPushConstantRange());
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createShadowProgram() {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(_logicalDevice, SHADERS_PATH "shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(_logicalDevice, SHADERS_PATH "shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    PushConstants pushConstants(_logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsShadow>(VK_SHADER_STAGE_VERTEX_BIT);
    return GraphicsShaderProgram::create<VertexP>(*this, std::move(shaders), {}, pushConstants.getVkPushConstantRange());
}

const VkDescriptorSetLayout ShaderProgramManager::getVkDescriptorSetLayout(DescriptorSetType type) const {
    return _descriptorSetLayouts.find(type)->second.getVkDescriptorSetLayout();
}

const DescriptorSetLayout* ShaderProgramManager::getDescriptorSetLayout(DescriptorSetType type) const {
    return &_descriptorSetLayouts.find(type)->second;
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateBindlessLayout() {
    static constexpr DescriptorSetType layoutType = DescriptorSetType::BINDLESS;
    if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
        return it->first;
    }
    DescriptorSetLayout bindlessDescriptorSetLayout(_logicalDevice);
    VkDescriptorBindingFlags flags{ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };
    bindlessDescriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 200, flags);
    bindlessDescriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL, 200, flags);
    RETURN_IF_ERROR(bindlessDescriptorSetLayout.build(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
    _descriptorSetLayouts.emplace(layoutType, std::move(bindlessDescriptorSetLayout));
    return layoutType;
}

ErrorOr<DescriptorSetType> ShaderProgramManager::getOrCreateCameraLayout() {
    static constexpr DescriptorSetType layoutType = DescriptorSetType::CAMERA;
    if (auto it = _descriptorSetLayouts.find(layoutType); it != _descriptorSetLayouts.cend()) {
        return it->first;
    }
    DescriptorSetLayout descriptorSetLayout(_logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    _descriptorSetLayouts.emplace(layoutType, std::move(descriptorSetLayout));
    return layoutType;
}

const LogicalDevice& ShaderProgramManager::getLogicalDevice() const {
    return _logicalDevice;
}