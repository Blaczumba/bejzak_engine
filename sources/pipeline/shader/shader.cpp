#include "shader.h"

#include "logical_device/logical_device.h"

#include <fstream>
#include <stdexcept>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    const size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

Shader::Shader(const LogicalDevice& logicalDevice, const std::string& shaderPath, const VkShaderStageFlagBits shaderStage) : _logicalDevice(logicalDevice), _shaderStage(shaderStage), _name("main") {
    const auto shaderCode = readFile(shaderPath);
    const VkDevice device = _logicalDevice.getVkDevice();
    const VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    if (vkCreateShaderModule(device, &createInfo, nullptr, &_shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
}

Shader::~Shader() {
    vkDestroyShaderModule(_logicalDevice.getVkDevice(), _shaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo Shader::getVkPipelineStageCreateInfo() const {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = _shaderStage,
        .module = _shaderModule,
        .pName = _name.data()
    };
}

VkShaderStageFlagBits Shader::getVkShaderStageFlagBits() const {
    return _shaderStage;
}
