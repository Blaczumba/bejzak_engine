#include "shader_program.h"

#include "descriptor_set/descriptor_set_layout.h"
#include "primitives/vk_primitives.h"
#include "primitives/primitives.h"

#include <algorithm>
#include <iterator>
#include <memory>

ShaderProgram::ShaderProgram(const LogicalDevice& logicalDevice, std::vector<Shader>&& shaders, std::unique_ptr<DescriptorSetLayout>&& descriptorSetLayout)
    : _logicalDevice(logicalDevice), _shaders(std::move(shaders)), _descriptorSetLayout(std::move(descriptorSetLayout)) {}

const DescriptorSetLayout& ShaderProgram::getDescriptorSetLayout() const {
    return *_descriptorSetLayout;
}

const PushConstants& ShaderProgram::getPushConstants() const {
    return _pushConstants;
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderProgram::getVkPipelineShaderStageCreateInfos() const {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(_shaders.size());
    std::transform(_shaders.cbegin(), _shaders.cend(), std::back_inserter(shaderStages), [](const Shader& shader) { return shader.getVkPipelineStageCreateInfo(); });
    return shaderStages;
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createNormalMappingProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(4);
    //TODO .value() to delete
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.tse.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_blinn_phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();   // TODO return error
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexPTN{});
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createPBRProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexPTNT{});
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

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createPBRTesselationProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(4);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.tsc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.tse.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr_tesselation.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexPTNT{});
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createSkyboxProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexP{});
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createShadowProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexP{});
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createSkyboxOffscreenProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "skybox_offscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexPTNT{});
}

std::unique_ptr<GraphicsShaderProgram> ShaderProgramFactory::createPBROffscreenProgram(const LogicalDevice& logicalDevice) {
    std::vector<Shader> shaders;
    shaders.reserve(2);
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "shader_pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT).value());
    shaders.push_back(Shader::create(logicalDevice, SHADERS_PATH "offscreen_shader_pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT).value());

    auto descriptorSetLayout = DescriptorSetLayout::create(logicalDevice);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    descriptorSetLayout->addLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    descriptorSetLayout->build();
    return std::make_unique<GraphicsShaderProgram>(logicalDevice, std::move(shaders), std::move(descriptorSetLayout), VertexP{});
}
