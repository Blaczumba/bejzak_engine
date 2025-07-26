#include "shader.h"

#include "vulkan_wrapper/lib/buffer/buffer.h"
#include "vulkan_wrapper/logical_device/logical_device.h"

#include <filesystem>
#include <fstream>
#include <string_view>

namespace {

ErrorOr<lib::Buffer<char>> readFile(std::string_view filename) {
    std::ifstream file(std::filesystem::path(SHADERS_PATH) / filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return Error(EngineError::LOAD_FAILURE);
    }

    const size_t fileSize = static_cast<size_t>(file.tellg());
    lib::Buffer<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

} // namespace

Shader::Shader(VkShaderModule shaderModule, const LogicalDevice& logicalDevice, VkShaderStageFlagBits shaderStage)
    : _shaderModule(shaderModule), _logicalDevice(&logicalDevice), _shaderStage(shaderStage) {
}

ErrorOr<Shader> Shader::create(const LogicalDevice& logicalDevice, std::string_view shaderPath, VkShaderStageFlagBits shaderStage) {
    ASSIGN_OR_RETURN(const lib::Buffer<char> shaderCode, readFile(shaderPath));

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    VkShaderModule shaderModule;

    if (VkResult result = vkCreateShaderModule(logicalDevice.getVkDevice(), &createInfo, nullptr, &shaderModule); result != VK_SUCCESS) {
        return Error(result);
    }
    return Shader(shaderModule, logicalDevice, shaderStage);
}

Shader::Shader(Shader&& other) noexcept
    : _shaderModule(std::exchange(other._shaderModule, VK_NULL_HANDLE)),
    _logicalDevice(other._logicalDevice),
    _shaderStage(other._shaderStage) {
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    _shaderModule = std::exchange(other._shaderModule, VK_NULL_HANDLE);
    _logicalDevice = other._logicalDevice;
    _shaderStage = other._shaderStage;
    return *this;
}

Shader::~Shader() {
    if (_shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_logicalDevice->getVkDevice(), _shaderModule, nullptr);
    }
}

VkPipelineShaderStageCreateInfo Shader::getVkPipelineStageCreateInfo() const {
    static constexpr std::string_view pname = "main";
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = _shaderStage,
        .module = _shaderModule,
        .pName = pname.data()
    };
}

VkShaderStageFlagBits Shader::getVkShaderStageFlagBits() const {
    return _shaderStage;
}