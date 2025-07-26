#pragma once

#include "vulkan_lib/status/status.h"

#include <vulkan/vulkan.h>

#include <string>

class LogicalDevice;

class Shader {
    VkShaderModule _shaderModule;
    VkShaderStageFlagBits _shaderStage;
    const LogicalDevice* _logicalDevice;

    Shader(VkShaderModule shaderModule, const LogicalDevice& logicalDevice, VkShaderStageFlagBits shaderStage);

public:
    static ErrorOr<Shader> create(const LogicalDevice& logicalDevice, std::string_view shaderPath, VkShaderStageFlagBits shaderStage);

    ~Shader();

    Shader(Shader&& other) noexcept;

    Shader& operator=(Shader&& other) noexcept;

    Shader(const Shader&) = delete;

    Shader& operator=(const Shader&) = delete;

    VkPipelineShaderStageCreateInfo getVkPipelineStageCreateInfo() const;

    VkShaderStageFlagBits getVkShaderStageFlagBits() const;
};