#include "shader_program.h"

#include "descriptor_set/descriptor_set_layout.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "pipeline/input_description.h"
#include "primitives/primitives.h"

#include <algorithm>
#include <iterator>
#include <memory>

ShaderProgram::ShaderProgram(const ShaderProgramManager& shaderProgramManager, std::initializer_list<std::string_view> shaders, std::initializer_list<DescriptorSetType> descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange, std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputInfo)
    : _shaderProgramManager(shaderProgramManager), _shaders(shaders), _descriptorSetLayouts(descriptorSetLayouts), _pushConstants(pushConstantRange.cbegin(), pushConstantRange.cend()), _vertexInputInfo(vertexInputInfo) {}

lib::Buffer<VkDescriptorSetLayout> ShaderProgram::getVkDescriptorSetLayouts() const {
    lib::Buffer<VkDescriptorSetLayout> layouts(_descriptorSetLayouts.size());
    std::transform(_descriptorSetLayouts.cbegin(), _descriptorSetLayouts.cend(), layouts.begin(), [this](DescriptorSetType layoutType) { return _shaderProgramManager.getVkDescriptorSetLayout(layoutType); });
    return layouts;
}

lib::Buffer<VkPipelineShaderStageCreateInfo> ShaderProgram::getVkPipelineShaderStageCreateInfos() const {
    lib::Buffer<VkPipelineShaderStageCreateInfo> shaders(_shaders.size());
    std::transform(_shaders.cbegin(), _shaders.cend(), shaders.begin(), [this](std::string_view shaderPath) { return _shaderProgramManager.getShader(shaderPath)->getVkPipelineStageCreateInfo(); });
    return shaders;
}

std::span<const DescriptorSetType> ShaderProgram::getDescriptorSetLayouts() const {
    return _descriptorSetLayouts;
}

std::span<const VkPushConstantRange> ShaderProgram::getPushConstants() const {
    return _pushConstants;
}

const std::optional<VkPipelineVertexInputStateCreateInfo>& ShaderProgram::getVkPipelineVertexInputStateCreateInfo() const {
    return _vertexInputInfo;
}

ErrorOr<std::unique_ptr<ShaderProgram>> ShaderProgramManager::createPBRProgram() {
    static constexpr std::string_view vertexShaderPath = "shader_pbr.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "shader_pbr.frag.spv";
    RETURN_IF_ERROR(addShader(vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    RETURN_IF_ERROR(addShader(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

    static constexpr VkPushConstantRange pushConstantRanges[] = {
        getPushConstantRange<PushConstantsPBR>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());
    ASSIGN_OR_RETURN(DescriptorSetType cameraLayout, getOrCreateCameraLayout());

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexPTNT>();

    return std::unique_ptr<ShaderProgram>(new ShaderProgram(*this, { vertexShaderPath, fragmentShaderPath }, { bindlessLayout, cameraLayout }, pushConstantRanges, vertexInputInfo));
}

ShaderProgramManager::ShaderProgramManager(const LogicalDevice& logicalDevice) : _logicalDevice(logicalDevice) { }

ErrorOr<std::unique_ptr<ShaderProgram>> ShaderProgramManager::createSkyboxProgram() {
    static constexpr std::string_view vertexShaderPath = "skybox.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "skybox.frag.spv";
    RETURN_IF_ERROR(addShader(vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    RETURN_IF_ERROR(addShader(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

    ASSIGN_OR_RETURN(DescriptorSetType bindlessLayout, getOrCreateBindlessLayout());

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexP>();

    static constexpr VkPushConstantRange pushConstantRanges[] = {
        getPushConstantRange<PushConstantsSkybox>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    return std::unique_ptr<ShaderProgram>(new ShaderProgram(*this, { vertexShaderPath, fragmentShaderPath }, { bindlessLayout }, pushConstantRanges, vertexInputInfo));
}

ErrorOr<std::unique_ptr<ShaderProgram>> ShaderProgramManager::createShadowProgram() {
    static constexpr std::string_view vertexShaderPath = "shadow.vert.spv";
    static constexpr std::string_view fragmentShaderPath = "shadow.frag.spv";
    RETURN_IF_ERROR(addShader(vertexShaderPath, VK_SHADER_STAGE_VERTEX_BIT));
    RETURN_IF_ERROR(addShader(fragmentShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT));

    static constexpr VkPushConstantRange pushConstantRanges[] = {
        getPushConstantRange<PushConstantsShadow>(VK_SHADER_STAGE_VERTEX_BIT)
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = getVkPipelineVertexInputStateCreateInfo<VertexP>();

    return std::unique_ptr<ShaderProgram>(new ShaderProgram(*this, { vertexShaderPath, fragmentShaderPath }, {}, pushConstantRanges, vertexInputInfo));
}

const VkDescriptorSetLayout ShaderProgramManager::getVkDescriptorSetLayout(DescriptorSetType type) const {
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

Status ShaderProgramManager::addShader(std::string_view shaderFile, VkShaderStageFlagBits shaderStages) {
    ASSIGN_OR_RETURN(Shader shader, Shader::create(_logicalDevice, shaderFile, shaderStages));
    _shaders.emplace(shaderFile, std::move(shader));
    return StatusOk();
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