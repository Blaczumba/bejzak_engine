#pragma once

#include <span>
#include <vulkan/vulkan.h>

#include "common/status/status.h"

class LogicalDevice;

class Shader {
  VkShaderModule _shaderModule;
  VkShaderStageFlagBits _shaderStage;
  const LogicalDevice* _logicalDevice;

  Shader(VkShaderModule shaderModule, const LogicalDevice& logicalDevice,
         VkShaderStageFlagBits shaderStage);

public:
  static ErrorOr<Shader> create(
      const LogicalDevice& logicalDevice, std::span<const std::byte> shaderData,
      VkShaderStageFlagBits shaderStage);

  ~Shader();

  Shader(Shader&& other) noexcept;

  Shader& operator=(Shader&& other) noexcept;

  Shader(const Shader&) = delete;

  Shader& operator=(const Shader&) = delete;

  VkPipelineShaderStageCreateInfo getVkPipelineStageCreateInfo() const;

  VkShaderStageFlagBits getVkShaderStageFlagBits() const;
};
