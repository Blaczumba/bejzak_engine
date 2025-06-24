#pragma once

#include "status/status.h"

#include <vulkan/vulkan.h>

#include <string>

class LogicalDevice;

class Shader {
    VkShaderModule _shaderModule;
    VkShaderStageFlagBits _shaderStage;
    std::string _name;
    const LogicalDevice* _logicalDevice;

    Shader(VkShaderModule shaderModule, const LogicalDevice& logicalDevice, VkShaderStageFlagBits shaderStage);

public:
    static ErrorOr<Shader> create(const LogicalDevice& logicalDevice, const std::string& shaderPath, VkShaderStageFlagBits shaderStage);

    ~Shader();

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    VkPipelineShaderStageCreateInfo getVkPipelineStageCreateInfo() const;
    VkShaderStageFlagBits getVkShaderStageFlagBits() const;
};