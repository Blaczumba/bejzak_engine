#pragma once

#include <glm/glm.hpp>
#include <string_view>
#include <unordered_map>

#include "common/file/file_loader.h"
#include "common/util/resource_handles.h"
#include "lib/sparse/sparse_map.h"
#include "lib/types/strong_int.h"
#include "vulkan/resource_manager/hasher.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set_layout.h"
#include "vulkan/wrapper/pipeline/graphics_pipeline_builder.h"
#include "vulkan/wrapper/pipeline/shader.h"

enum class DescriptorSetType : uint8_t {
  BINDLESS,
  CAMERA,
  COMPUTE
};

class PipelineManager {
  static constexpr size_t MAX_PIPELINE_LAYOUTS = 32;

  struct PipelineLayoutID {
    PipelineLayout layout;
    size_t refCount;
  };

  using PipelineLayoutMap = lib::SparseMap<PipelineLayoutID, MAX_PIPELINE_LAYOUTS>;
  using PipelineLayoutMapIndex = typename PipelineLayoutMap::IndexType;

  struct PipelineResource {
    Pipeline pipeline;
    PipelineLayoutMapIndex layoutIndex;
  };

  using PipelineMap = lib::SparseMap<PipelineResource, MAX_PIPELINES>;

  PipelineManager(const FileLoader& fileLoader);

public:
  static std::unique_ptr<PipelineManager> create(const FileLoader& fileLoader);

  ~PipelineManager() = default;

  std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
  getOrCreateBindlessLayout(const LogicalDevice& logicalDevice);

  std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
  getOrCreateCameraLayout(const LogicalDevice& logicalDevice, bool multiview);

  std::pair<VkDescriptorSetLayout, std::reference_wrapper<DescriptorSetLayoutMetadata>>
  getOrCreateComputeLayout(const LogicalDevice& logicalDevice);

  Pipeline* getPipeline(PipelineHandle index);

  bool removePipeline(PipelineHandle index);

  PipelineHandle createPBRProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview);

  PipelineHandle createPbrTesselationProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview);

  PipelineHandle createBlinnPhongTesselationProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview);

  PipelineHandle createPbrEnvMappingProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout);

  PipelineHandle createEnvMappingProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout, bool multiview);

  PipelineHandle createSkyboxProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout);

  PipelineHandle createShadowProgram(
      const Renderpass& renderpass, const AttachmentLayout& attachmentLayout);

  PipelineHandle createFragmentShadingRateProgram(const LogicalDevice& logicalDevice);

private:
  PipelineLayoutMap _pipelineLayouts;
  std::vector<PipelineLayoutMapIndex> _freePipelineLayoutIndices;

  std::unordered_map<PipelineLayoutKey, PipelineLayoutMapIndex, PipelineLayoutHasher>
      _pipelineLayoutIndices;
  std::unordered_map<PipelineLayoutMapIndex, PipelineLayoutKey> _pipelineLayoutKeys;

  PipelineMap _pipelines;
  std::vector<PipelineHandle> _freePipelineIndices;

  const FileLoader& _fileLoader;

  std::unordered_map<std::string_view, Shader> _shaders;
  std::unordered_map<DescriptorSetType, std::pair<DescriptorSetLayout, DescriptorSetLayoutMetadata>>
      _descriptorSetLayouts;

  const Shader& addShader(const LogicalDevice& logicalDevice, std::string_view shaderFile,
                          VkShaderStageFlagBits shaderStages);

  std::pair<PipelineLayout*, PipelineLayoutMapIndex> getOrCreatePipelineLayout(
      const PipelineLayoutKey& key, const LogicalDevice& logicalDevice);

  bool removePipelineLayout(PipelineLayoutMapIndex index);
};

struct PushConstantsModelDescriptorHandles16bit {
  glm::mat4 model;
  uint16_t descriptorHandles[32];
};

struct PushConstantsModelDescriptorHandles32Bit {
  glm::mat4 model;
  uint32_t descriptorHandles[16];
};

struct PushConstantFov {
  glm::u32vec2 pixelSpaceViewDir;
};

struct PushConstantsShadow {
  glm::mat4 model;
  glm::mat4 lightProjView;
};

struct PushConstantsPhongEnv {
  glm::mat4 lightProjView;
  glm::mat3x4 model;
  uint32_t envMapHandle;
};

struct PushConstantsSkybox {
  glm::mat4 proj;
  glm::mat3x4 view;
  uint32_t skyboxHandle;
};
