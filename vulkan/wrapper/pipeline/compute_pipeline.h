#include <string>

#include "pipeline.h"
#include "vulkan/wrapper/logical_device/logical_device.h"

class ComputePipeline : public Pipeline {
  const LogicalDevice& _logicalDevice;

public:
  ComputePipeline(const LogicalDevice& logicalDevice, VkDescriptorSetLayout descriptorSetLayout,
                  const std::string& computeShader);
  ~ComputePipeline();
};
