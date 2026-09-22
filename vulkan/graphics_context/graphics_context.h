#pragma once

#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "common/entity_component_system/component/material.h"
#include "common/entity_component_system/component/mesh.h"
#include "common/entity_component_system/registry/registry.h"
#include "common/model_loader/model_loader.h"
#include "common/object/object.h"
#include "common/scene/octree.h"
#include "common/util/primitives.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/presentation_lib.h"
#include "vulkan/resource_manager/asset_manager.h"
#include "vulkan/resource_manager/bindless_descriptor_set_writer.h"
#include "vulkan/resource_manager/buffer_manager.h"
#include "vulkan/resource_manager/framebuffer_manager.h"
#include "vulkan/resource_manager/image_manager.h"
#include "vulkan/resource_manager/pipeline_manager.h"
#include "vulkan/resource_manager/sampler_manager.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger.h"
#include "vulkan/wrapper/descriptor_set/descriptor_pool.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set_writer.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/render_pass/render_pass.h"
#include "vulkan/wrapper/synchronization/fence.h"

inline VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback1(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  std::cerr << "[Vulkan Validation] " << "Severity: " << messageSeverity << ", "
            << "Type: " << messageType << std::endl
            << "Message: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

namespace vlkn {

template <bool SYNCED_OUTSIDE, bool MULTIVIEW_PRESENTATION>
class GraphicsContext final : public common::GraphicsContext {
  GraphicsContext(std::shared_ptr<Instance> instance, DebugMessenger&& debugMessenger,
                  std::unique_ptr<PhysicalDevice> physicalDevice,
                  std::unique_ptr<LogicalDevice> logicalDevice, const FileLoader& fileLoader,
                  std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
                  std::unique_ptr<PresentationContext> presentationContext = nullptr);

public:
  static std::unique_ptr<common::GraphicsContext> create(
      std::shared_ptr<Instance> instance, DebugMessenger&& debugMessenger,
      std::unique_ptr<PhysicalDevice> physicalDevice, std::unique_ptr<LogicalDevice> logicalDevice,
      const FileLoader& fileLoader,
      std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
      std::unique_ptr<PresentationContext> presentationContext);

  ~GraphicsContext();

  void draw() override;

  void initializeResources() override;

  void waitCompleteExecution() const override;

  void createPresentingResources(const common::PresentResources& presentResources) override;

  void waitDeviceIdle() const override;

  static constexpr uint32_t MAX_THREADS_IN_POOL = 2;

private:
  std::shared_ptr<Instance> _instance;
  DebugMessenger _debugMessenger;
  std::unique_ptr<PhysicalDevice> _physicalDevice;
  std::unique_ptr<LogicalDevice> _logicalDevice;
  std::unique_ptr<PresentationContext> _presentationContext;
  std::shared_ptr<engine::PresentationGraphicsCommunication> _communicationLayer;

  const FileLoader& _fileLoader;

  uint8_t _currentFrame = 0;

  std::array<Fence, MAX_FRAMES_IN_FLIGHT> _frameFences;

  std::shared_ptr<CommandPool> _singleTimeCommandPool;

  BufferManager _bufferManager;
  ImageManager _imageManager;
  FramebufferManager _framebufferManager;
  std::unique_ptr<AssetManager> _assetManager;
  std::unique_ptr<SamplerManager> _samplerManager;
  std::unique_ptr<PipelineManager> _pipelineManager;

  std::shared_ptr<DescriptorPool> _bindlessDescriptorPool;
  DescriptorSet _bindlessDescriptorSet;
  std::unique_ptr<BindlessDescriptorSetWriter> _bindlessWriter;

  std::shared_ptr<DescriptorPool> _descriptorPool;
  DescriptorSet _dynamicDescriptorSet;
  DescriptorSetWriter _dynamicDescriptorSetWriter;
  DescriptorSet _computeDescriptorSet;
  DescriptorSetWriter _computeDescriptorSetWriter;

  // Temporal objects for demonstration:
  std::array<std::shared_ptr<CommandPool>, MAX_THREADS_IN_POOL + 1> _commandPools;
  std::array<CommandBuffer, MAX_FRAMES_IN_FLIGHT> _primaryCommandBuffer;
  std::array<std::array<CommandBuffer, MAX_FRAMES_IN_FLIGHT>, MAX_THREADS_IN_POOL>
      _secondaryCommandBuffers;

  std::vector<Object> _objects;
  std::unique_ptr<Octree> _octree;
  Registry _registry;

  Renderpass _renderPass;
  AttachmentLayout _attachmentLayout;
  std::vector<Ref<Framebuffer>> _framebuffers;

  // Shadowmap
  Renderpass _shadowRenderPass;
  AttachmentLayout _shadowAttachmentLayout;
  Ref<Framebuffer> _shadowFramebuffer;
  Ref<Image> _shadowMapRef;
  Pipeline* _shadowPipeline;
  UniformTextureHandle _shadowHandle;

  // Skybox.
  Entity _skyboxEntity;
  Pipeline* _skyboxPipeline;

  // Raziel.
  Entity _razielEntity;

  // Mirror cubemap
  // First pass.
  Renderpass _envMappingRenderPass;
  AttachmentLayout _envMappingAttachmentLayout;
  Ref<Framebuffer> _envMappingFramebuffer;
  Pipeline* _envMappingPipeline;
  Buffer _envMappingUniformBuffer;
  UniformBufferHandle _envMappingHandle;
  std::array<Image, 2> _envMappingAttachments;
  UniformTextureHandle _envMappingTextureHandle;
  // Second pass.
  Pipeline* _phongEnvMappingPipeline;

  // PBR objects.
  std::vector<Object> objects;
  Pipeline* _graphicsPipeline;
  PipelineHandle _graphicsPipelineHandle;
  Pipeline* _graphicsTesselationPipeline;
  PipelineHandle _graphicsTesselationPipelineHandle;

  // Blinn Phong Tesselation.
  PipelineHandle _blinnPhongTesselationPipelineHandle;

  UniformBufferLight _ubLight;
  std::tuple<Buffer, BufferMetadata> _dynamicUniformBuffersCamera;
  Buffer _lightBuffer;
  UniformBufferHandle _lightHandle;

  // Fragment rate shading.
  Pipeline* _fsrPipeline;
  Ref<Image> _fsrTextureHandle;

  void setup();

  Entity loadObject(VkCommandBuffer commandBuffer, const common::AssetData& cubeData,
                    PipelineHandle pipelineHandle, Image&& image, const ImageMetadata& metadata);

  void createDescriptorSets();

  void createEnvMappingResources();

  void createShadowResources();

  void createGraphicsPipelines();

  void createCommandBuffers();

  void createSyncObjects();

  std::tuple<UniformTextureHandle, ImageHandle> getOrLoadTexture(
      std::unordered_map<uint64_t, std::pair<UniformTextureHandle, ImageHandle>>& textureCache,
      const AssetManager::ImageData& textureID, VkFormat format, const CommandBuffer& commandBuffer,
      float maxSamplerAnisotropy, Ref<Sampler>& samplerRef);

  void loadObjects(std::span<const common::AssetData> sceneData, PipelineHandle pipelineHandle);

  void createOctreeScene();

  void recordShadowCommandBuffer(const CommandBuffer& commandBuffer);

  void recordEnvMappingCommandBuffer(const CommandBuffer& commandBuffer);

  void updateUniformBuffer(uint32_t currentFrame);

  void recordOctreeSecondaryCommandBuffer(
      const CommandBuffer& commandBuffer, const OctreeNode* rootNode,
      std::span<const glm::vec4> planes, std::span<VkDescriptorSet> descriptorSets,
      std::span<uint32_t> dynamicUniformBufferOffsets);

  void recordCommandBuffer(const glm::mat4& cameraProj, const glm::mat4& cameraView,
                           uint32_t imageIndex, glm::u32vec2 screenPos);
};

// The two specializations are explicitly instantiated in graphics_context.cpp.
// Declaring them extern here prevents other translation units from trying to
// implicitly instantiate the member definitions.
extern template class GraphicsContext<false, false>;

extern template class GraphicsContext<true, true>;

}  // namespace vlkn
