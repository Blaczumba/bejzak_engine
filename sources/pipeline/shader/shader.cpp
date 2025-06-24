#include "shader.h"

#include "lib/buffer/buffer.h"
#include "logical_device/logical_device.h"

#include <fstream>

namespace {

ErrorOr<lib::Buffer<char>> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return Error(EngineError::LOAD_FAILURE);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    lib::Buffer<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

} // namespace

Shader::Shader(VkShaderModule shaderModule, const LogicalDevice& logicalDevice, VkShaderStageFlagBits shaderStage)
    : _shaderModule(shaderModule), _logicalDevice(&logicalDevice), _shaderStage(shaderStage), _name("main") {
}

ErrorOr<Shader> Shader::create(const LogicalDevice& logicalDevice, const std::string& shaderPath, VkShaderStageFlagBits shaderStage) {
    ASSIGN_OR_RETURN(const lib::Buffer<char> shaderCode, readFile(shaderPath));

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkDevice device = logicalDevice.getVkDevice();
    VkShaderModule shaderModule;

    if (VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule); result != VK_SUCCESS) {
        return Error(result);
    }

    return Shader(shaderModule, logicalDevice, shaderStage);
}

Shader::Shader(Shader&& other) noexcept
    : _shaderModule(std::exchange(other._shaderModule, VK_NULL_HANDLE)),
    _logicalDevice(other._logicalDevice),
    _shaderStage(other._shaderStage),
    _name(std::move(other._name)) {
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    _shaderModule = std::exchange(other._shaderModule, VK_NULL_HANDLE);
    _logicalDevice = other._logicalDevice;
    _shaderStage = other._shaderStage;
    _name = std::move(other._name);
}

Shader::~Shader() {
    if (_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_logicalDevice->getVkDevice(), _shaderModule, nullptr);
    }
}

VkPipelineShaderStageCreateInfo Shader::getVkPipelineStageCreateInfo() const {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = _shaderStage,
        .module = _shaderModule,
        .pName = _name.c_str()
    };
}

VkShaderStageFlagBits Shader::getVkShaderStageFlagBits() const {
    return _shaderStage;
}