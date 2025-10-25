#pragma once

#include <memory>
#include <vulkan/vulkan.h>

class Pipeline {
protected:
  VkPipeline _pipeline = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
  VkPipelineBindPoint _pipelineBindPoint;

public:
  Pipeline(VkPipelineBindPoint pipelineBindPoint);
  virtual ~Pipeline() = default;

  VkPipeline getVkPipeline() const;
  VkPipelineLayout getVkPipelineLayout() const;
  VkPipelineBindPoint getVkPipelineBindPoint() const;
};
