#include "shader_program.h"

#include "descriptor_set/descriptor_set_layout.h"
#include "lib/macros/status_macros.h"
#include "logical_device/logical_device.h"
#include "primitives/vk_primitives.h"
#include "primitives/primitives.h"

#include <algorithm>
#include <iterator>
#include <memory>

ShaderProgram::ShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::vector<DescriptorSetLayout>&& descriptorSetLayouts, std::span<const VkPushConstantRange> pushConstantRange)
    : _logicalDevice(logicalDevice), _shaders(std::move(shaders)), _descriptorSetLayouts(std::move(descriptorSetLayouts)), _pushConstants(pushConstantRange.cbegin(), pushConstantRange.cend()) {}

std::span<const DescriptorSetLayout> ShaderProgram::getDescriptorSetLayouts() const {
    return _descriptorSetLayouts;
}

std::span<const Shader> ShaderProgram::getShaders() const {
    return _shaders;
}

std::span<const VkPushConstantRange> ShaderProgram::getPushConstants() const {
    return _pushConstants;
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createNormalMappingProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader tesselationControlShader, Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT));
    ASSIGN_OR_RETURN(Shader tesselationEvaluationShader, Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.tse.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(4);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(tesselationControlShader));
    shaders.push_back(std::move(tesselationEvaluationShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.build();
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    return GraphicsShaderProgram::create<VertexPTN>(logicalDevice, std::move(shaders), std::move(v));
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createPBRProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));

    DescriptorSetLayout pbrDescriptorSetLayout1(logicalDevice);
    VkDescriptorBindingFlags flags{ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };
    // descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, {});
    pbrDescriptorSetLayout1.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 200, flags);
    pbrDescriptorSetLayout1.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL, 200, flags);
    RETURN_IF_ERROR(pbrDescriptorSetLayout1.build(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));

    PushConstants pushConstants(logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsPBR>(VK_SHADER_STAGE_ALL);

    return GraphicsShaderProgram::create<VertexPTNT>(logicalDevice, std::move(shaders), std::move(v), pushConstants.getVkPushConstantRange());
    //std::vector<Shader> shaders;
    //shaders.reserve(2);
    //shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    //shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    //auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    //VkDescriptorBindingFlags flags{ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };
    //// descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, {});
    //descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 200, flags);
    //descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL, 200, flags);
    //descriptorSetLayout->build(VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
    //return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexPTNT{});
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createPBRTesselationProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader tesselationControlShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT));
    ASSIGN_OR_RETURN(Shader tesselationEvaluationShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.tse.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(4);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(tesselationControlShader));
    shaders.push_back(std::move(tesselationEvaluationShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    return GraphicsShaderProgram::create<VertexPTNT>(logicalDevice, std::move(shaders), std::move(v));
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createSkyboxProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    auto descriptorSetLayout = DescriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    return GraphicsShaderProgram::create<VertexP>(logicalDevice, std::move(shaders), std::move(v));
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createShadowProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    PushConstants pushConstants(logicalDevice.getPhysicalDevice());
    pushConstants.addPushConstant<PushConstantsShadow>(VK_SHADER_STAGE_VERTEX_BIT);
    return GraphicsShaderProgram::create<VertexP>(logicalDevice, std::move(shaders), std::move(v), pushConstants.getVkPushConstantRange());
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createSkyboxOffscreenProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "skybox_offscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    return GraphicsShaderProgram::create<VertexPTNT>(logicalDevice, std::move(shaders), std::move(v));
}

ErrorOr<std::unique_ptr<GraphicsShaderProgram>> ShaderProgramManager::createPBROffscreenProgram(const LogicalDevice& logicalDevice) {
    ASSIGN_OR_RETURN(Shader vertexShader, Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    ASSIGN_OR_RETURN(Shader fragmentShader, Shader::create(logicalDevice, SHADERS_PATH "offscreen_shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(std::move(vertexShader));
    shaders.push_back(std::move(fragmentShader));

    DescriptorSetLayout descriptorSetLayout(logicalDevice);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout.addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    RETURN_IF_ERROR(descriptorSetLayout.build());
    std::vector<DescriptorSetLayout> v;
    v.push_back(std::move(descriptorSetLayout));
    return GraphicsShaderProgram::create<VertexP>(logicalDevice, std::move(shaders), std::move(v));
}
