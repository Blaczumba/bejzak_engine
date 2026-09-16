#include "vulkan/graphics_context/graphics_context.h"

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <span>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/abstractions/contexts.h"
#include "common/abstractions/graphics_context.h"
#include "common/entity_component_system/component/material.h"
#include "common/entity_component_system/component/mesh.h"
#include "common/entity_component_system/component/transform.h"
#include "common/entity_component_system/registry/registry.h"
#include "common/model_loader/model_loader.h"
#include "common/model_loader/obj_loader/obj_loader.h"
#include "common/model_loader/tiny_gltf_loader/tiny_gltf_loader.h"
#include "common/object/object.h"
#include "common/scene/octree.h"
#include "common/util/engine_exception.h"
#include "common/util/primitives.h"
#include "lib/bitwise.h"
#include "presentation_graphics_communication/presentation_graphics_communication.h"
#include "vulkan/graphics_context/graphics_context_lib.h"
#include "vulkan/graphics_context/presentation_lib.h"
#include "vulkan/resource_manager/asset_manager.h"
#include "vulkan/resource_manager/bindless_descriptor_set_writer.h"
#include "vulkan/resource_manager/framebuffer_attachments_manager.h"
#include "vulkan/resource_manager/pipeline_manager.h"
#include "vulkan/resource_manager/sampler_manager.h"
#include "vulkan/wrapper/builders/dependency_info_builder.h"
#include "vulkan/wrapper/builders/image_memory_barrier_builder.h"
#include "vulkan/wrapper/builders/submit_info_builder.h"
#include "vulkan/wrapper/command_buffer/command_buffer.h"
#include "vulkan/wrapper/command_buffer/single_time_command_buffer.h"
#include "vulkan/wrapper/debug_messenger/debug_messenger.h"
#include "vulkan/wrapper/descriptor_set/descriptor_pool.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set.h"
#include "vulkan/wrapper/descriptor_set/descriptor_set_writer.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/instance/instance.h"
#include "vulkan/wrapper/logical_device/logical_device.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/memory_objects/memory_objects_lib.h"
#include "vulkan/wrapper/physical_device/physical_device.h"
#include "vulkan/wrapper/render_pass/render_pass.h"
#include "vulkan/wrapper/synchronization/fence.h"
#include "vulkan/wrapper/util/index_buffer_util.h"

#define GCONTEXT_TEMPLATE template <bool SYNCED_OUTSIDE, bool MULTIVIEW_PRESENTATION>
#define GCONTEXT_CLASS    GraphicsContext<SYNCED_OUTSIDE, MULTIVIEW_PRESENTATION>::

namespace vlkn {

namespace {

Ref<Buffer> copyStagingToGpuBuffer(const LogicalDevice& logicalDevice, BufferManager* bufferManager,
                                   const VkCommandBuffer commandBuffer, Ref<Buffer> bufferRef,
                                   Ref<VirtualAllocation> allocationRef, VkBufferUsageFlags usage) {
  auto [buffer, metadata] =
      BufferBuilder()
          .withUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage)
          .withSize(allocationRef.getMetadata().size)
          .buildVertexInputBufferWithMetadata(logicalDevice);
  copyBufferToBuffer(commandBuffer, bufferRef.getVkResource(), buffer.getVkBuffer(),
                     allocationRef.getMetadata().offset, 0, allocationRef.getMetadata().size);
  return bufferManager->storeBuffer(std::move(buffer), metadata);
}

std::tuple<Framebuffer, FramebufferMetadata> createFramebufferFromTextures(
    const Renderpass& renderpass,
    std::span<const std::tuple<VkImageView, ImageMetadata>> imageViews) {
  FramebufferBuilder builder;
  std::optional<VkExtent2D> extent;
  for (const auto& [imageView, metadata] : imageViews) {
    builder.addAttachment(imageView);
    if (!extent.has_value()) {
      extent = VkExtent2D{metadata.imageExtent.width, metadata.imageExtent.height};
    } else if (
        VkExtent2D tmpExtent = VkExtent2D{metadata.imageExtent.width, metadata.imageExtent.height};
        extent->width != tmpExtent.width || extent->height != tmpExtent.height) {
      throw EngineException("All images must have the same size to create a Framebuffer.");
    }
  }

  if (!extent.has_value()) {
    throw EngineException("Framebuffer must have an attachment.");
  }

  Framebuffer framebuffer = builder.build(renderpass, *extent, 1);
  return std::make_pair(std::move(framebuffer), builder.getMetadata());
}

std::tuple<Image, ImageMetadata> createSkybox(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    const AssetManager::ImageData& imageData, VkFormat format);

std::tuple<Image, ImageMetadata> createCubemap(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    VkImageAspectFlags aspect, VkFormat format, VkImageUsageFlags additionalUsage);

std::tuple<Image, ImageMetadata> createShadowmap(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer, uint32_t width,
    uint32_t height, VkFormat format);

std::tuple<Image, ImageMetadata> createTexture2D(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    const AssetManager::ImageData& imageData, VkFormat format, float samplerAnisotropy);

std::tuple<Image, ImageMetadata> createAttachment(
    const LogicalDevice& logicalDevice, VkFormat format, VkSampleCountFlagBits samples,
    VkExtent2D extent, uint32_t numLayers, VkImageAspectFlags aspect, VkImageUsageFlags usage);

void createFsrContents(const LogicalDevice& logicalDevice, Image& image,
                       const ImageMetadata& metadata, const CommandBuffer& commandBuffer);

}  // namespace

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS setup() {
  const std::vector<common::AssetData> sponzaData =
      common::LoadGltfFromFile(*_assetManager, _fileLoader, MODELS_PATH "sponza/scene.gltf");
  //     std::vector<common::VertexData> antiqueCandleStickData = common::LoadGltfFromFile(
  //         *_assetManager, MODELS_PATH "ornate_antique_candlestick/scene.gltf");
  //     std::for_each(
  //         antiqueCandleStickData.begin(), antiqueCandleStickData.end(), [](common::VertexData&
  //         data) {
  //           data.model = data.model * glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, -2.0f,
  //           .0f))
  //                        * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f));
  //         });
  /*std::vector<common::VertexData> lanternData =
      common::LoadGltfFromFile(*_assetManager, MODELS_PATH "ornate_lantern_3d_model/scene.gltf");
  std::for_each(lanternData.begin(), lanternData.end(), [](common::VertexData& data) {
    data.model = data.model * glm::translate(glm::mat4(1.0f), glm::vec3(8.5f, 4.25f, 0.0f))
                 * glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
  });*/
  // std::vector<common::VertexData> spartanData =
  //     common::LoadGltfFromFile(*_assetManager, MODELS_PATH "pbr_spartan_helmet/scene.gltf");
  // std::for_each(spartanData.begin(), spartanData.end(), [](common::VertexData& data) {
  //   data.model = data.model * glm::translate(glm::mat4(1.0f), glm::vec3(1000.0f, 16.0f,
  //   -250.0f))
  //                * glm::rotate(glm::mat4(1.0f), glm::radians(-25.0f), glm::vec3(1.0f, 0.0f,
  //                0.0f))
  //                * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
  // });

  createDescriptorSets();
  createEnvMappingResources();
  createShadowResources();
  createGraphicsPipelines();
  createCommandBuffers();
  createSyncObjects();
  loadObjects(sponzaData, _graphicsPipelineHandle);
  // loadObjects(antiqueCandleStickData, _graphicsPipelineHandle);
  // loadObjects(lanternData, _graphicsTesselationPipelineHandle);
  // loadObjects(spartanData, _graphicsTesselationPipelineHandle);

  {
    SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
    const VkCommandBuffer commandBuffer = handle.getVkCommandBuffer();

    std::string cubeFileContents = _fileLoader.loadFileToString(MODELS_PATH "cube.obj");
    common::AssetData cubeData = common::loadObj(*_assetManager, "cube.obj", cubeFileContents);
    common::waitForAssetToLoad(cubeData.vertexData->loadState);
    cubeData.diffuseTexture = {
      _assetManager->loadImageAsync([this]() {
        static constexpr std::string_view filePath = TEXTURES_PATH "cubemap_yokohama_rgba.ktx";
        return loadImage(_fileLoader.loadFileToBuffer(filePath), filePath);
      }),
      TEXTURES_PATH "cubemap_yokohama_rgba.ktx"};
    common::waitForAssetToLoad(cubeData.diffuseTexture.imageData->loadState);
    auto [skyboxImage, skyboxMetadata] = createSkybox(
        *_logicalDevice, handle, *cubeData.diffuseTexture.imageData, VK_FORMAT_R8G8B8A8_SRGB);
    _skyboxEntity = loadObject(
        commandBuffer, cubeData, PipelineHandle(0), std::move(skyboxImage), skyboxMetadata);

    // std::string razielFileContents = _fileLoader.loadFileToString(MODELS_PATH "Raziel.obj");
    // common::VertexData razielData =
    //     common::loadObj(*_assetManager, "Raziel.obj", razielFileContents);
    // razielData.model =
    //     glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    //     * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 3.0f));
    // razielData.diffuseTexture = {
    //   _assetManager->loadImageAsync(TEXTURES_PATH "Raziel.png"), TEXTURES_PATH "Raziel.png"};
    // const AssetManager::ImageData& razielImageData =
    //     _assetManager->getImageData(razielData.diffuseTexture.ID);

    // Texture razielTexture =
    //     createTexture2D(*_logicalDevice, commandBuffer, razielImageData,
    //     VK_FORMAT_R8G8B8A8_SRGB,
    //                     _physicalDevice->getMaxSamplerAnisotropy());
    //_razielEntity = loadObject(commandBuffer, razielData, _blinnPhongTesselationPipelineHandle,
    //                            std::move(razielTexture));
    //_objects.push_back(Object("Raziel", _razielEntity));
  }

  createOctreeScene();
  {
    SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
    recordShadowCommandBuffer(handle);
    recordEnvMappingCommandBuffer(handle);
  }
}

GCONTEXT_TEMPLATE
Entity GCONTEXT_CLASS loadObject(
    VkCommandBuffer commandBuffer, const common::AssetData& cubeData, PipelineHandle pipelineHandle,
    Image&& image, const ImageMetadata& metadata) {
  Entity entity = _registry.createEntity();
  Ref<Sampler> samplerRef = _samplerManager->getOrCreateSampler(
      *_logicalDevice, SamplerBuilder()
                           .withMaxAnisotropy(_physicalDevice->getMaxSamplerAnisotropy())
                           .buildMetadata());
  VkImageView view = image.getVkImageView();
  Ref<Image> imageRef = _imageManager->storeImage(std::move(image), metadata);
  _registry.addComponent<MaterialComponent>(
      entity,
      MaterialComponent{.diffuse = _bindlessWriter->writeTexture(
                            imageRef, samplerRef, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            _samplerManager->getVkResource(samplerRef.getHandle())),
                        .pipelineHandle = pipelineHandle});
  Ref<Buffer> vertexBufferRef;
  Ref<VirtualAllocation> virtualAllocationRef;
  std::tie(vertexBufferRef, virtualAllocationRef) = cubeData.vertexData->buffers.at("P");
  MeshComponent msh = {
    .aabb = createAABBfromVertices(
        std::span(reinterpret_cast<glm::vec3*>(vertexBufferRef.getMetadata().mappedMemory
                                               + virtualAllocationRef.getMetadata().offset),
                  virtualAllocationRef.getMetadata().size / sizeof(glm::vec3)),
        glm::mat4(1.0f))};
  Ref<Buffer> pBufferRef;
  Ref<VirtualAllocation> pVirtualAllocationRef;
  std::tie(pBufferRef, pVirtualAllocationRef) = cubeData.vertexData->buffers.at("P");
  msh.vertexBufferPrimitiveHandle = copyStagingToGpuBuffer(
      *_logicalDevice, _bufferManager.get(), commandBuffer, pBufferRef, pVirtualAllocationRef,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  Ref<Buffer> ptnBufferRef;
  Ref<VirtualAllocation> ptnVirtualAllocationRef;
  std::tie(ptnBufferRef, ptnVirtualAllocationRef) = cubeData.vertexData->buffers.at("PTN");
  msh.vertexBufferHandle = copyStagingToGpuBuffer(
      *_logicalDevice, _bufferManager.get(), commandBuffer, ptnBufferRef, ptnVirtualAllocationRef,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  Ref<Buffer> indexBufferRef;
  Ref<VirtualAllocation> indexVirtualAllocationRef;
  std::tie(indexBufferRef, indexVirtualAllocationRef) = cubeData.vertexData->indexBuffer;
  msh.indexBufferHandle = copyStagingToGpuBuffer(
      *_logicalDevice, _bufferManager.get(), commandBuffer, indexBufferRef,
      indexVirtualAllocationRef, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  msh.indexType = internal::convertIndexTypeToVkIndexType(cubeData.vertexData->indexType);
  _registry.addComponent(entity, std::move(msh));
  _registry.addComponent(entity, TransformComponent{.model = cubeData.model});
  return entity;
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createDescriptorSets() {
  // If VR presentation is enabled then multiply times 2 otherwise times 1.
  const uint32_t size =
      _logicalDevice->getPhysicalDevice().getMemoryAlignment(sizeof(UniformBufferCamera));
  {
    _dynamicUniformBuffersCamera =
        BufferBuilder()
            .withUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            .withSize((MULTIVIEW_PRESENTATION ? 2 : 1) * MAX_FRAMES_IN_FLIGHT * size)
            .buildUniformBufferWithMetadata(*_logicalDevice);
  }

  _dynamicDescriptorSetWriter.storeDynamicBuffer(
      std::get<Buffer>(_dynamicUniformBuffersCamera),
      std::get<BufferMetadata>(_dynamicUniformBuffersCamera).usage, size,
      MULTIVIEW_PRESENTATION ? 2 : 1);
  _dynamicDescriptorSetWriter.writeDescriptorSet(
      _logicalDevice->getVkDevice(), _dynamicDescriptorSet.getVkDescriptorSet());

  {
    auto [buffer, metadata] =
        BufferBuilder()
            .withUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            .withSize(sizeof(UniformBufferLight))
            .buildUniformBufferWithMetadata(*_logicalDevice);
    Ref<Buffer> lightBufferRef = _bufferManager->storeBuffer(std::move(buffer), metadata);
    _lightHandle = _bindlessWriter->writeBuffer(
        lightBufferRef, _bufferManager->getVkResource(lightBufferRef.getHandle()), metadata);

    _ubLight.pos = glm::vec3(15.1891f, 2.66408f, -0.841221f);
    _ubLight.projView = glm::perspective(glm::radians(120.0f), 1.0f, 0.1f, 40.0f);
    _ubLight.projView[1][1] = -_ubLight.projView[1][1];
    _ubLight.projView = _ubLight.projView
                        * glm::lookAt(_ubLight.pos, glm::vec3(-3.82383f, 3.66503f, 1.30751f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
    common::copyObject(metadata.getMappedMemoryAsSpan(), _ubLight);
    _lightBuffer = std::move(buffer);
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createEnvMappingResources() {
  // First pass for rendering the environment map.
  // const float samplerAnisotropy = _physicalDevice->getMaxSamplerAnisotropy();
  //{
  //  SingleTimeCommandBuffer handle(*_singleTimeCommandPool);

  //  _envMappingAttachments = createCubemap(
  //      *_logicalDevice, handle.getVkCommandBuffer(), VK_IMAGE_ASPECT_COLOR_BIT,
  //      VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, samplerAnisotropy);
  //  _envMappingAttachments[1] = createCubemap(
  //      *_logicalDevice, handle.getVkCommandBuffer(), VK_IMAGE_ASPECT_DEPTH_BIT,
  //      VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, samplerAnisotropy);
  //}

  //_envMappingAttachmentLayout.addColorAttachment(
  //    VK_FORMAT_R8G8B8A8_SRGB, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
  //_envMappingAttachmentLayout.addDepthAttachment(
  //    VK_FORMAT_D16_UNORM, VK_ATTACHMENT_STORE_OP_DONT_CARE);

  // RenderpassBuilder renderpassBuilder(_envMappingAttachmentLayout);
  // renderpassBuilder.createSubpass().addOutputAttachment(0).addOutputAttachment(1);
  //_envMappingRenderPass =
  //     renderpassBuilder.withMultiView({0b111111}, {0b111111})
  //         .addDependency(
  //             VK_SUBPASS_EXTERNAL, 0,
  //             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  //                 | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
  //             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
  //             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  //             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  //                 | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
  //             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
  //             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
  //         .build(*_logicalDevice);

  // auto [framebuffer, metadata] =
  //     createFramebufferFromTextures(_envMappingRenderPass, _envMappingAttachments);
  //_envMappingFramebuffer = _framebufferAttachmentManager->storeFramebuffer(
  //     std::move(framebuffer), metadata, {});  // TODO: pass proper attachments

  // const glm::vec3 pos = glm::vec3(0.0f, 2.0f, 0.0f);
  // glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 50.0f);

  // struct {
  //   alignas(16) glm::mat4 projView[6];
  //   alignas(16) glm::vec3 viewPos;
  //   alignas(16) glm::mat4 lightProjView;
  //   alignas(16) glm::vec3 lightPos;
  // } const faceTransform = {
  //   .projView =
  //       {
  //                  proj * glm::lookAt(pos, pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,
  //                  -1.0f, 0.0f)), proj * glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f),
  //                  glm::vec3(0.0f, -1.0f, 0.0f)), proj * glm::lookAt(pos, pos +
  //                  glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), proj *
  //                  glm::lookAt(pos, pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f,
  //                  -1.0f)), proj * glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, 1.0f),
  //                  glm::vec3(0.0f, -1.0f, 0.0f)), proj * glm::lookAt(pos, pos + glm::vec3(0.0f,
  //                  0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
  //                  },
  //   .viewPos = pos,
  //   .lightProjView = _ubLight.projView,
  //   .lightPos = _ubLight.pos
  // };

  // TODO:
  //_envMappingUniformBuffer = Buffer::createUniformBuffer(*_logicalDevice, sizeof(faceTransform));
  // common::copyData(_envMappingUniformBuffer.getMappedMemory(), 0, faceTransform);
  //_envMappingHandle = _bindlessWriter->writeBuffer(_envMappingUniformBuffer);
  // Sampler sampler = SamplerBuilder().withAnisotropy(samplerAnisotropy).build(*_logicalDevice);
  //_envMappingTextureHandle = _bindlessWriter->writeTexture(
  //    _envMappingAttachments[0].getVkImageView(), _envMappingAttachments[0].getVkImageLayout(),
  //    sampler.getVkSampler());
  //_samplerManager->transferSampler(std::move(sampler));
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createShadowResources() {
  VkImageView view;
  {
    // TODO: Should not be in this function.
    SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
    const VkCommandBuffer commandBuffer = handle.getVkCommandBuffer();
    auto [image, metadata] =
        createShadowmap(*_logicalDevice, handle, 1024 * 2, 1024 * 2, VK_FORMAT_D32_SFLOAT);
    view = image.getVkImageView();
    _shadowMapRef = _imageManager->storeImage(std::move(image), metadata);
  }
  Ref<Sampler> samplerRef = _samplerManager->getOrCreateSampler(
      *_logicalDevice,
      SamplerBuilder()
          .withCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
          .withAddressMode(
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
              VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
          .withMinMagFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR)
          .withBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
          .buildMetadata());
  _shadowHandle = _bindlessWriter->writeTexture(
      _shadowMapRef, samplerRef, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      _samplerManager->getVkResource(samplerRef.getHandle()));
  _shadowAttachmentLayout.addShadowAttachment(
      VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  RenderpassBuilder builder(_shadowAttachmentLayout);
  builder.createSubpass().addOutputAttachment(0);
  _shadowRenderPass = builder.build(*_logicalDevice);
  const std::tuple<VkImageView, ImageMetadata> shadowMapData = {view, _shadowMapRef.getMetadata()};
  auto [framebuffer, framebufferMetadata] =
      createFramebufferFromTextures(_shadowRenderPass, std::span(&shadowMapData, 1));
  _shadowFramebuffer = _framebufferAttachmentManager->storeFramebuffer(
      std::move(framebuffer), framebufferMetadata, {&_shadowMapRef, 1});  // TODO pass proper
                                                                          // attachments
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createGraphicsPipelines() {
  _graphicsPipelineHandle =
      _pipelineManager->createPBRProgram(_renderPass, _attachmentLayout, MULTIVIEW_PRESENTATION);
  _graphicsPipeline = _pipelineManager->getPipeline(_graphicsPipelineHandle);
  _graphicsTesselationPipelineHandle = _pipelineManager->createPbrTesselationProgram(
      _renderPass, _attachmentLayout, MULTIVIEW_PRESENTATION);
  _blinnPhongTesselationPipelineHandle = _pipelineManager->createBlinnPhongTesselationProgram(
      _renderPass, _attachmentLayout, MULTIVIEW_PRESENTATION);
  _graphicsTesselationPipeline = _pipelineManager->getPipeline(_graphicsTesselationPipelineHandle);
  _skyboxPipeline = _pipelineManager->getPipeline(
      _pipelineManager->createSkyboxProgram(_renderPass, _attachmentLayout));
  _phongEnvMappingPipeline =
      _pipelineManager->getPipeline(_pipelineManager->createEnvMappingProgram(
          _renderPass, _attachmentLayout, MULTIVIEW_PRESENTATION));
  _shadowPipeline = _pipelineManager->getPipeline(
      _pipelineManager->createShadowProgram(_shadowRenderPass, _shadowAttachmentLayout));
  //_envMappingPipeline =
  //_pipelineManager->getPipeline(_pipelineManager->createPbrEnvMappingProgram(
  //    _envMappingRenderPass, _envMappingAttachmentLayout));
  _fsrPipeline = _pipelineManager->getPipeline(
      _pipelineManager->createFragmentShadingRateProgram(*_logicalDevice));
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createCommandBuffers() {
  for (int i = 0; i <= MAX_THREADS_IN_POOL; i++) {
    _commandPools[i] =
        CommandPoolBuilder()
            .withFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
            .withQueueFamilyIndex(*_physicalDevice->getQueueFamilyIndices().graphicsFamily)
            .build(*_logicalDevice);
  }
  _primaryCommandBuffer =
      _commandPools[MAX_THREADS_IN_POOL]->createCommandBuffers<MAX_FRAMES_IN_FLIGHT>(
          VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  for (int i = 0; i < MAX_THREADS_IN_POOL; i++) {
    _secondaryCommandBuffers[i] = _commandPools[i]->createCommandBuffers<MAX_FRAMES_IN_FLIGHT>(
        VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createSyncObjects() {
  for (Fence& fence : _frameFences) {
    fence = FenceBuilder().build(*_logicalDevice, VK_FENCE_CREATE_SIGNALED_BIT);
  }
}

GCONTEXT_TEMPLATE
std::tuple<UniformTextureHandle, ImageHandle> GCONTEXT_CLASS getOrLoadTexture(
    std::unordered_map<uint64_t, std::pair<UniformTextureHandle, ImageHandle>>& textureCache,
    const AssetManager::ImageData& imageData, VkFormat format, const CommandBuffer& commandBuffer,
    float maxSamplerAnisotropy, Ref<Sampler>& samplerRef) {
  common::waitForAssetToLoad(imageData.loadState);
  auto [it, inserted] = textureCache.try_emplace(
      (static_cast<uint64_t>(imageData.stagingBuffer.getHandle()) << 32)
      | imageData.virtualAllocation.getHandle());

  if (!inserted) {
    return it->second;
  }

  auto [image, metadata] =
      createTexture2D(*_logicalDevice, commandBuffer, imageData, format, maxSamplerAnisotropy);
  VkImageView view = image.getVkImageView();
  Ref<Image> imageRef = _imageManager->storeImage(std::move(image), metadata);
  UniformTextureHandle handle = _bindlessWriter->writeTexture(
      imageRef, samplerRef, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      _samplerManager->getVkResource(samplerRef.getHandle()));

  const auto result = std::make_tuple(handle, imageRef.getHandle());
  it->second = result;

  return result;
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS loadObjects(
    std::span<const common::AssetData> sceneData, PipelineHandle pipelineHandle) {
  const float maxSamplerAnisotropy = _physicalDevice->getMaxSamplerAnisotropy();

  std::unordered_map<uint64_t, std::pair<UniformTextureHandle, ImageHandle>> textureCache;
  textureCache.reserve(sceneData.size());
  SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
  VkCommandBuffer commandBuffer = handle.getVkCommandBuffer();
  Ref<Sampler> samplerRef = _samplerManager->getOrCreateSampler(
      *_logicalDevice, SamplerBuilder()
                           .withMaxAnisotropy(_physicalDevice->getMaxSamplerAnisotropy())
                           .withLodRange(0.0f, VK_LOD_CLAMP_NONE)
                           .buildMetadata());

  for (const common::AssetData& sceneObject : sceneData) {
    const auto [diffuseHandle, diffuseTextureIndex] =
        getOrLoadTexture(textureCache, *sceneObject.diffuseTexture.imageData,
                         VK_FORMAT_R8G8B8A8_SRGB, handle, maxSamplerAnisotropy, samplerRef);

    const auto [normalHandle, normalTextureIndex] =
        getOrLoadTexture(textureCache, *sceneObject.normalTexture.imageData,
                         VK_FORMAT_R8G8B8A8_UNORM, handle, maxSamplerAnisotropy, samplerRef);

    const auto [metallicRoughnessHandle, metallicRoughnessTextureIndex] =
        getOrLoadTexture(textureCache, *sceneObject.metallicRoughnessTexture.imageData,
                         VK_FORMAT_R8G8B8A8_UNORM, handle, maxSamplerAnisotropy, samplerRef);

    Entity e = _registry.createEntity();
    _objects.emplace_back("", e);
    _registry.addComponent<MaterialComponent>(
        e, MaterialComponent{diffuseHandle, normalHandle, metallicRoughnessHandle, pipelineHandle});
    MeshComponent msh;
    const AssetManager::VertexData& vData = *sceneObject.vertexData;
    common::waitForAssetToLoad(vData.loadState);
    Ref<Buffer> ptntBufferRef;
    Ref<VirtualAllocation> ptntVirtualAllocationRef;
    std::tie(ptntBufferRef, ptntVirtualAllocationRef) = vData.buffers.at("PTNT");
    msh.vertexBufferHandle = copyStagingToGpuBuffer(
        *_logicalDevice, _bufferManager.get(), commandBuffer, ptntBufferRef,
        ptntVirtualAllocationRef, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    Ref<Buffer> pBufferRef;
    Ref<VirtualAllocation> pVirtualAllocationRef;
    std::tie(pBufferRef, pVirtualAllocationRef) = vData.buffers.at("P");
    msh.vertexBufferPrimitiveHandle = copyStagingToGpuBuffer(
        *_logicalDevice, _bufferManager.get(), commandBuffer, pBufferRef, pVirtualAllocationRef,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    msh.aabb = createAABBfromVertices(
        std::span(reinterpret_cast<glm::vec3*>(pBufferRef.getMetadata().mappedMemory
                                               + pVirtualAllocationRef.getMetadata().offset),
                  pVirtualAllocationRef.getMetadata().size / sizeof(glm::vec3)),
        sceneObject.model);
    Ref<Buffer> indexBufferRef;
    Ref<VirtualAllocation> indexVirtualAllocationRef;
    std::tie(indexBufferRef, indexVirtualAllocationRef) = vData.indexBuffer;
    msh.indexBufferHandle = copyStagingToGpuBuffer(
        *_logicalDevice, _bufferManager.get(), commandBuffer, indexBufferRef,
        indexVirtualAllocationRef, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    msh.indexType = internal::convertIndexTypeToVkIndexType(vData.indexType);

    _registry.addComponent<MeshComponent>(e, std::move(msh));

    TransformComponent trsf;
    trsf.model = sceneObject.model;
    _registry.addComponent<TransformComponent>(e, std::move(trsf));
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createOctreeScene() {
  if (_objects.empty()) {
    return;
  }

  AABB sceneAABB = _registry.getComponent<MeshComponent>(_objects[0].getEntity()).aabb;

  for (int i = 1; i < _objects.size(); ++i) {
    sceneAABB.extend(_registry.getComponent<MeshComponent>(_objects[i].getEntity()).aabb);
  }
  _octree = std::make_unique<Octree>(sceneAABB);

  for (const Object& object : _objects) {
    _octree->addObject(&object, _registry.getComponent<MeshComponent>(object.getEntity()).aabb);
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS recordShadowCommandBuffer(const CommandBuffer& commandBuffer) {
  const ImageMetadata& metadata = _shadowMapRef.getMetadata();
  commandBuffer.setVieport({
    VkViewport{.width = static_cast<float>(metadata.imageExtent.width),
               .height = static_cast<float>(metadata.imageExtent.height),
               .minDepth = 0.0f,
               .maxDepth = 1.0f}
  });
  const VkExtent2D extent{metadata.imageExtent.width, metadata.imageExtent.height};
  commandBuffer.setScissor({VkRect2D{.extent = extent}});

  const Framebuffer& framebuffer =
      _framebufferAttachmentManager->getFramebuffer(_shadowFramebuffer);
  commandBuffer.beginRenderPass(
      VK_SUBPASS_CONTENTS_INLINE, framebuffer, extent, _shadowAttachmentLayout.getVkClearValues());

  commandBuffer.bindPipeline(
      _shadowPipeline->getVkPipelineBindPoint(), _shadowPipeline->getVkPipeline());

  PushConstantsShadow pc = {.lightProjView = _ubLight.projView};

  for (const Object& object : _objects) {
    const MeshComponent& meshComponent = _registry.getComponent<MeshComponent>(object.getEntity());
    const TransformComponent& transformComponent =
        _registry.getComponent<TransformComponent>(object.getEntity());

    pc.model = transformComponent.model;
    commandBuffer.pushConstants(_shadowPipeline->getVkPipelineLayout(),
                                _shadowPipeline->getPushConstantVkShaderStageFlags(),
                                std::span(reinterpret_cast<const std::byte*>(&pc), sizeof(pc)));

    commandBuffer.bindVertexBuffers(
        {_bufferManager->getVkResource(
            BufferHandle(meshComponent.vertexBufferPrimitiveHandle.getHandle()))},
        {0});

    commandBuffer.bindIndexBuffer(
        _bufferManager->getVkResource(BufferHandle(meshComponent.indexBufferHandle.getHandle())),
        meshComponent.indexType);
    commandBuffer.drawIndexed(WeakRef<Buffer>(meshComponent.indexBufferHandle).getMetadata().size
                                  / getIndexSize(meshComponent.indexType),
                              1);
  }
  commandBuffer.endRenderPass();
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS recordEnvMappingCommandBuffer(const CommandBuffer& commandBuffer) {
  // const VkExtent2D extent = _envMappingAttachments[0].getVkExtent2D();
  // commandBuffer.setVieport({
  //   VkViewport{.width = static_cast<float>(extent.width),
  //              .height = static_cast<float>(extent.height),
  //              .minDepth = 0.0f,
  //              .maxDepth = 1.0f}
  // });
  // commandBuffer.setScissor({VkRect2D{.extent = extent}});

  // const Framebuffer& framebuffer =
  //     _framebufferAttachmentManager->getFramebuffer(_envMappingFramebuffer);
  // commandBuffer.beginRenderPass(VK_SUBPASS_CONTENTS_INLINE, framebuffer, extent,
  //                               _envMappingAttachmentLayout.getVkClearValues());

  // commandBuffer.bindPipeline(
  //     _envMappingPipeline->getVkPipelineBindPoint(), _envMappingPipeline->getVkPipeline());

  // commandBuffer.bindDescriptorSets(
  //     _envMappingPipeline->getVkPipelineBindPoint(), _envMappingPipeline->getVkPipelineLayout(),
  //     {_bindlessDescriptorSet.getVkDescriptorSet()});

  // for (const Object& object : _objects) {
  //   const MeshComponent& meshComponent =
  //   _registry.getComponent<MeshComponent>(object.getEntity()); const TransformComponent&
  //   transformComponent =
  //       _registry.getComponent<TransformComponent>(object.getEntity());
  //   const MaterialComponent& materialComponent =
  //       _registry.getComponent<MaterialComponent>(object.getEntity());

  //  const PushConstantsModelDescriptorHandles32Bit pc = {
  //    .model = transformComponent.model,
  //    .descriptorHandles = {static_cast<uint32_t>(*_envMappingHandle),
  //                          static_cast<uint32_t>(*materialComponent.diffuse),
  //                          static_cast<uint32_t>(*materialComponent.normal),
  //                          static_cast<uint32_t>(*materialComponent.metallicRoughness),
  //                          static_cast<uint32_t>(*_shadowHandle)}
  //  };
  //  commandBuffer.pushConstants(_envMappingPipeline->getVkPipelineLayout(),
  //                              _envMappingPipeline->getPushConstantVkShaderStageFlags(),
  //                              std::span(reinterpret_cast<const std::byte*>(&pc), sizeof(pc)));

  //  commandBuffer.bindVertexBuffers(
  //      {_gpuBufferManager->getBuffer(meshComponent.vertexBufferHandle).buffer.getVkBuffer()},
  //      {0});

  //  const auto& [indexBuffer, indexBufferMetadata] =
  //      _gpuBufferManager->getBuffer(meshComponent.indexBufferHandle);
  //  commandBuffer.bindIndexBuffer(indexBuffer.getVkBuffer(), meshComponent.indexType);
  //  commandBuffer.drawIndexed(indexBufferMetadata.size / getIndexSize(meshComponent.indexType),
  //  1);
  //}
  // commandBuffer.endRenderPass();
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS updateUniformBuffer(uint32_t currentFrame) {
  UniformBufferCamera _ubCamera;
  std::span<const common::CameraContext> cameraContexts = _communicationLayer->getCameraContexts();
  // TODO: Switch to std::views::enumerate.
  for (size_t i = 0; i < cameraContexts.size(); i++) {
    const common::CameraContext& cameraContext = cameraContexts[i];
    _ubCamera.view = cameraContext.view;
    _ubCamera.proj = cameraContext.proj;
    _ubCamera.pos = cameraContext.position;
    _ubCamera.viewDir = cameraContext.viewDir;
    common::copyObject(
        std::get<BufferMetadata>(_dynamicUniformBuffersCamera).getMappedMemoryAsSpan(), _ubCamera,
        (cameraContexts.size() * currentFrame + i)
            * _physicalDevice->getMemoryAlignment(sizeof(_ubCamera)));
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS recordOctreeSecondaryCommandBuffer(
    const CommandBuffer& commandBuffer, const OctreeNode* rootNode,
    std::span<const glm::vec4> planes, std::span<VkDescriptorSet> descriptorSets,
    std::span<uint32_t> dynamicUniformBufferOffsets) {
  if (!rootNode || !rootNode->getVolume().intersectsFrustum(planes)) {
    return;
  }

  std::optional<PipelineHandle> globalPipelineHandle;
  Pipeline* pipeline;
  static std::queue<const OctreeNode*> nodeQueue;  // Keep it static to preserve
  // capacity
  nodeQueue.push(rootNode);

  while (!nodeQueue.empty()) {
    const OctreeNode* node = nodeQueue.front();
    nodeQueue.pop();

    for (const Object* object : node->getObjects()) {
      const auto& materialComponent =
          _registry.getComponent<MaterialComponent>(object->getEntity());
      const auto& transformComponent =
          _registry.getComponent<TransformComponent>(object->getEntity());

      const PushConstantsModelDescriptorHandles32Bit pc = {
        .model = transformComponent.model,
        .descriptorHandles = {
                              static_cast<uint32_t>(*_lightHandle), static_cast<uint32_t>(*materialComponent.diffuse),
                              static_cast<uint32_t>(*materialComponent.normal),
                              static_cast<uint32_t>(*materialComponent.metallicRoughness),
                              static_cast<uint32_t>(*_shadowHandle)}
      };

      if (!globalPipelineHandle.has_value()
          || materialComponent.pipelineHandle != *globalPipelineHandle) {
        globalPipelineHandle = materialComponent.pipelineHandle;
        pipeline = _pipelineManager->getPipeline(*globalPipelineHandle);
        commandBuffer.bindPipeline(pipeline->getVkPipelineBindPoint(), pipeline->getVkPipeline());
        commandBuffer.bindDescriptorSets(
            pipeline->getVkPipelineBindPoint(), pipeline->getVkPipelineLayout(), descriptorSets, 0,
            dynamicUniformBufferOffsets);
      }
      commandBuffer.pushConstants(
          pipeline->getVkPipelineLayout(), pipeline->getPushConstantVkShaderStageFlags(),
          std::span(reinterpret_cast<const std::byte*>(&pc), sizeof(pc)));
      const auto& meshComponent = _registry.getComponent<MeshComponent>(object->getEntity());
      const VkBuffer vertexBuffers[] = {
        _bufferManager->getVkResource(BufferHandle(meshComponent.vertexBufferHandle.getHandle()))};
      static constexpr VkDeviceSize offsets[] = {0};
      commandBuffer.bindVertexBuffers(vertexBuffers, offsets);
      commandBuffer.bindIndexBuffer(
          _bufferManager->getVkResource(BufferHandle(meshComponent.indexBufferHandle.getHandle())),
          meshComponent.indexType);
      vkCmdDrawIndexed(
          commandBuffer.getVkCommandBuffer(),
          _bufferManager->getMetadata(BufferHandle(meshComponent.indexBufferHandle.getHandle()))
                  .size
              / getIndexSize(meshComponent.indexType),
          1, 0, 0, 0);
    }

    static constexpr OctreeNode::Subvolume options[] = {
      OctreeNode::Subvolume::LOWER_LEFT_BACK,  OctreeNode::Subvolume::LOWER_LEFT_FRONT,
      OctreeNode::Subvolume::LOWER_RIGHT_BACK, OctreeNode::Subvolume::LOWER_RIGHT_FRONT,
      OctreeNode::Subvolume::UPPER_LEFT_BACK,  OctreeNode::Subvolume::UPPER_LEFT_FRONT,
      OctreeNode::Subvolume::UPPER_RIGHT_BACK, OctreeNode::Subvolume::UPPER_RIGHT_FRONT};

    for (OctreeNode::Subvolume option : options) {
      const OctreeNode* childNode = node->getChild(option);
      if (childNode && childNode->getVolume().intersectsFrustum(planes)) {
        nodeQueue.push(childNode);
      }
    }
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS recordCommandBuffer(const glm::mat4& cameraProj, const glm::mat4& cameraView,
                                        uint32_t imageIndex, glm::u32vec2 screenPos) {
  const CommandBuffer& primaryCommandBuffer = _primaryCommandBuffer[_currentFrame];
  CommandBuffer::BeginInfoBuilder().beginCommandBuffer(
      primaryCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  const PushConstantFov fsrPc = {screenPos};
  primaryCommandBuffer.pushConstants(
      _fsrPipeline->getVkPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT,
      std::span{reinterpret_cast<const std::byte*>(&fsrPc), sizeof(fsrPc)});
  primaryCommandBuffer.bindPipeline(
      _fsrPipeline->getVkPipelineBindPoint(), _fsrPipeline->getVkPipeline());
  primaryCommandBuffer.bindDescriptorSets(
      _fsrPipeline->getVkPipelineBindPoint(), _fsrPipeline->getVkPipelineLayout(),
      {_computeDescriptorSet.getVkDescriptorSet()});
  primaryCommandBuffer.dispatchCompute(16, 16);

  const ImageMetadata& fsrTextureMetadata =
      _imageManager->getMetadata(_fsrTextureHandle.getHandle());
  VkImage fsrVkImage = _imageManager->getVkResource(_fsrTextureHandle.getHandle());
  static DependencyInfoBuilder dependencyInfoBuilder;
  dependencyInfoBuilder.clearBuilders()
      .addImageMemoryBarrier()
      .withSrcMasks(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT)
      .withDstMasks(VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR,
                    VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)
      .withLayouts(
          VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR)
      .withImage(fsrVkImage,
                 VkImageSubresourceRange{
                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                   .baseMipLevel = 0,
                   .levelCount = fsrTextureMetadata.mipLevels,
                   .baseArrayLayer = 0,
                   .layerCount = fsrTextureMetadata.arrayLayers,
                 });
  const VkDependencyInfo dependencyInfo = dependencyInfoBuilder.build();
  primaryCommandBuffer.pipelineBarrier(&dependencyInfo);

  const auto& [framebuffer, framebufferData] =
      _framebufferAttachmentManager->getFramebufferWithMetadata(_framebuffers[imageIndex]);
  const VkViewport viewports[] = {
    VkViewport{.width = static_cast<float>(framebufferData.metadata.extent.width),
               .height = static_cast<float>(framebufferData.metadata.extent.height),
               .minDepth = 0.0f,
               .maxDepth = 1.0f}
  };
  const VkRect2D scissors[] = {VkRect2D{.extent = framebufferData.metadata.extent}};
  primaryCommandBuffer.setVieport(viewports);
  primaryCommandBuffer.setScissor(scissors);
  primaryCommandBuffer.beginRenderPass(
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, framebuffer, framebufferData.metadata.extent,
      _attachmentLayout.getVkClearValues());

  auto beginInfoBuilder = CommandBuffer::BeginInfoBuilder().withInheritenceInfo(
      framebuffer.getRenderpass().getVkRenderPass(), framebuffer.getVkFramebuffer(), 0);
  static const bool viewportScissorInheritance =
      _physicalDevice->hasAvailableExtension(VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME);
  if (viewportScissorInheritance) [[likely]] {
    beginInfoBuilder.withViewportScissorInheritenceInfo(viewports);
  }

  std::future<void> futures[MAX_THREADS_IN_POOL];

  futures[0] = std::async(std::launch::async, [&]() -> void {
    const CommandBuffer& secondaryCommandBuffer = _secondaryCommandBuffers[0][_currentFrame];

    beginInfoBuilder.beginCommandBuffer(
        secondaryCommandBuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
                                    | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    if (!viewportScissorInheritance) [[unlikely]] {
      secondaryCommandBuffer.setVieport(viewports);
      secondaryCommandBuffer.setScissor(scissors);
    }

    const OctreeNode* root = _octree->getRoot();
    const auto& planes = extractFrustumPlanes(cameraProj * cameraView);

    VkDescriptorSet descriptorSets[] = {
      _bindlessDescriptorSet.getVkDescriptorSet(), _dynamicDescriptorSet.getVkDescriptorSet()};

    if constexpr (MULTIVIEW_PRESENTATION) {
      uint32_t dynamicUniformBufferOffsets[2];
      const uint32_t baseOffset = 2u * _currentFrame;
      _dynamicDescriptorSetWriter.getDynamicBufferSizesWithOffsets(
          dynamicUniformBufferOffsets, {baseOffset, baseOffset});
      recordOctreeSecondaryCommandBuffer(
          secondaryCommandBuffer, root, planes, descriptorSets, dynamicUniformBufferOffsets);
    } else {
      uint32_t dynamicUniformBufferOffset[1];
      _dynamicDescriptorSetWriter.getDynamicBufferSizesWithOffsets(
          dynamicUniformBufferOffset, {_currentFrame});
      recordOctreeSecondaryCommandBuffer(
          secondaryCommandBuffer, root, planes, descriptorSets, dynamicUniformBufferOffset);
    }

    CHECK_VKCMD(secondaryCommandBuffer.end(), "Failed to vkEndCommandBuffer.");
  });

  futures[1] = std::async(std::launch::async, [&]() -> void {
    // Skybox
    const CommandBuffer& secondaryCommandBuffer = _secondaryCommandBuffers[1][_currentFrame];

    beginInfoBuilder.beginCommandBuffer(
        secondaryCommandBuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
                                    | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    if (!viewportScissorInheritance) [[unlikely]] {
      secondaryCommandBuffer.setVieport(viewports);
      secondaryCommandBuffer.setScissor(scissors);
    }

    secondaryCommandBuffer.bindPipeline(
        _skyboxPipeline->getVkPipelineBindPoint(), _skyboxPipeline->getVkPipeline());

    const MeshComponent& cubeMeshComponent = _registry.getComponent<MeshComponent>(_skyboxEntity);
    const MaterialComponent& cubeMaterialComponent =
        _registry.getComponent<MaterialComponent>(_skyboxEntity);
    const VkBuffer vertexBuffers[] = {_bufferManager->getVkResource(
        BufferHandle(cubeMeshComponent.vertexBufferPrimitiveHandle.getHandle()))};
    static constexpr VkDeviceSize offsets[] = {0};
    secondaryCommandBuffer.bindVertexBuffers(vertexBuffers, offsets);
    secondaryCommandBuffer.bindIndexBuffer(
        _bufferManager->getVkResource(
            BufferHandle(cubeMeshComponent.indexBufferHandle.getHandle())),
        cubeMeshComponent.indexType);

    const PushConstantsSkybox pc = {
      .proj = cameraProj,
      .view = cameraView,
      .skyboxHandle = static_cast<uint32_t>(*cubeMaterialComponent.diffuse)};
    secondaryCommandBuffer.pushConstants(
        _skyboxPipeline->getVkPipelineLayout(),
        _skyboxPipeline->getPushConstantVkShaderStageFlags(),
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(&pc), sizeof(pc)));

    secondaryCommandBuffer.bindDescriptorSets(
        _skyboxPipeline->getVkPipelineBindPoint(), _skyboxPipeline->getVkPipelineLayout(),
        {_bindlessDescriptorSet.getVkDescriptorSet()});

    secondaryCommandBuffer.drawIndexed(
        _bufferManager->getMetadata(BufferHandle(cubeMeshComponent.indexBufferHandle.getHandle()))
                .size
            / getIndexSize(cubeMeshComponent.indexType),
        1);

    CHECK_VKCMD(secondaryCommandBuffer.end(), "Failed to vkEndCommandBuffer.");
  });

  std::for_each(std::begin(futures), std::end(futures), [](const std::future<void>& future) {
    future.wait();
  });

  const VkCommandBuffer secondaryCommandBuffers[] = {
    _secondaryCommandBuffers[0][_currentFrame].getVkCommandBuffer(),
    _secondaryCommandBuffers[1][_currentFrame].getVkCommandBuffer()};
  primaryCommandBuffer.executeSecondaryCommandBuffers(secondaryCommandBuffers);

  primaryCommandBuffer.endRenderPass();

  if (primaryCommandBuffer.end() != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

GCONTEXT_TEMPLATE
GCONTEXT_CLASS GraphicsContext(
    std::shared_ptr<Instance> instance, DebugMessenger&& debugMessenger,
    std::unique_ptr<PhysicalDevice> physicalDevice, std::unique_ptr<LogicalDevice> logicalDevice,
    const FileLoader& fileLoader,
    std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
    std::unique_ptr<PresentationContext> presentationContext)
  : _instance(std::move(instance)), _debugMessenger(std::move(debugMessenger)),
    _physicalDevice(std::move(physicalDevice)), _logicalDevice(std::move(logicalDevice)),
    _fileLoader(fileLoader), _communicationLayer(std::move(communicationLayer)),
    _presentationContext(std::move(presentationContext)) {
  _singleTimeCommandPool =
      CommandPoolBuilder()
          .withFlags(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
          .withQueueFamilyIndex(*_physicalDevice->getQueueFamilyIndices().graphicsFamily)
          .build(*_logicalDevice);
  _bufferManager = BufferManager::create();
  _imageManager = ImageManager::create();
  _assetManager = AssetManager::create(*_logicalDevice, *_bufferManager);
  _samplerManager = SamplerManager::create();
  _pipelineManager = PipelineManager::create(fileLoader);
  _framebufferAttachmentManager = FramebufferAttachmentManager::create();

  {
    _bindlessDescriptorPool =
        DescriptorPoolBuilder()
            .withFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
            .build(*_logicalDevice, 1);
    const auto [layout, metadata] = _pipelineManager->getOrCreateBindlessLayout(*_logicalDevice);
    _bindlessDescriptorSet =
        _bindlessDescriptorPool
            ->createDesriptorSet(
                layout, internal::getDescriptorPoolSizesFromBindings(metadata.get().bindings))
            .value();
    _bindlessWriter = BindlessDescriptorSetWriter::create(_bindlessDescriptorSet);
  }

  {
    _descriptorPool =
        DescriptorPoolBuilder()
            .withPoolSizes({
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2},
              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1}
    })
            .build(*_logicalDevice, 2);
    const auto [layout, metadata] =
        _pipelineManager->getOrCreateCameraLayout(*_logicalDevice, MULTIVIEW_PRESENTATION);
    _dynamicDescriptorSet =
        _descriptorPool
            ->createDesriptorSet(
                layout, internal::getDescriptorPoolSizesFromBindings(metadata.get().bindings))
            .value();
  }

  {
    const auto [layout, metadata] = _pipelineManager->getOrCreateComputeLayout(*_logicalDevice);
    _computeDescriptorSet =
        _descriptorPool
            ->createDesriptorSet(
                layout, internal::getDescriptorPoolSizesFromBindings(metadata.get().bindings))
            .value();
  }
}

GCONTEXT_TEMPLATE
std::unique_ptr<common::GraphicsContext> GCONTEXT_CLASS create(
    std::shared_ptr<Instance> instance, DebugMessenger&& debugMessenger,
    std::unique_ptr<PhysicalDevice> physicalDevice, std::unique_ptr<LogicalDevice> logicalDevice,
    const FileLoader& fileLoader,
    std::shared_ptr<engine::PresentationGraphicsCommunication> communicationLayer,
    std::unique_ptr<PresentationContext> presentationContext) {
  return std::unique_ptr<GraphicsContext<SYNCED_OUTSIDE, MULTIVIEW_PRESENTATION>>(
      new GraphicsContext<SYNCED_OUTSIDE, MULTIVIEW_PRESENTATION>(
          std::move(instance), std::move(debugMessenger), std::move(physicalDevice),
          std::move(logicalDevice), fileLoader, std::move(communicationLayer),
          std::move(presentationContext)));
}

GCONTEXT_TEMPLATE
GCONTEXT_CLASS ~GraphicsContext() {
  for (const Fence& fence : _frameFences) {
    fence.wait();
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS draw() {
  updateUniformBuffer(_currentFrame);

  CHECK_VKCMD(_primaryCommandBuffer[_currentFrame].resetCommandBuffer(),
              "Failed to reset primary command buffer.");
  for (int i = 0; i < MAX_THREADS_IN_POOL; i++) {
    CHECK_VKCMD(_secondaryCommandBuffers[i][_currentFrame].resetCommandBuffer(),
                "Failed to reset secondary command buffer.");
  }

  CHECK_VKCMD(_frameFences[_currentFrame].reset(), "Failed to wait for fence.");

  const common::CameraContext& cameraContext = _communicationLayer->getCameraContexts()[0];
  const auto [screenx, screeny] = _communicationLayer->getScreenPos();
  recordCommandBuffer(cameraContext.proj, cameraContext.view,
                      _communicationLayer->getCurrentSwapchainImageIndex(), {screenx, screeny});

  static SubmitInfoBuilder submitInfoBuilder;
  if constexpr (!SYNCED_OUTSIDE) {
    _presentationContext->synchronizeSubmit(&submitInfoBuilder);
  }

  CHECK_VKCMD(submitInfoBuilder
                  .withCommandBuffers({_primaryCommandBuffer[_currentFrame].getVkCommandBuffer()})
                  .submitQueue(_logicalDevice->getGraphicsVkQueue(),
                               _frameFences[_currentFrame].getVkFence()),
              "Failed to submit draw command buffer.");

  if (++_currentFrame == MAX_FRAMES_IN_FLIGHT) {
    _currentFrame = 0;
  }

  if constexpr (!SYNCED_OUTSIDE) {
    _presentationContext->setCurrentFrame(_currentFrame);
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS initializeResources() {
  setup();
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS waitCompleteExecution() const {
  CHECK_VKCMD(_frameFences[_currentFrame].wait(), "Failed to wait on VkFence.");
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS createPresentingResources(const common::PresentResources& presentResources) {
  lib::Buffer<VkPhysicalDeviceFragmentShadingRateKHR> fragmentShadingRates =
      _physicalDevice->getFragmentShadingRates();

  static constexpr VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_2_BIT;
  const VkFormat swapchainImageFormat = static_cast<VkFormat>(presentResources.imageFormat);
  const VkExtent2D extent = VkExtent2D{presentResources.width, presentResources.height};

  _attachmentLayout = AttachmentLayout(msaaSamples);
  _attachmentLayout
      .addColorResolvePresentAttachment(swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
      .addColorAttachment(
          swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
      .addDepthAttachment(VK_FORMAT_D24_UNORM_S8_UINT, VK_ATTACHMENT_STORE_OP_DONT_CARE)
      .addFragmentShadingRateAttachment();

  lib::Buffer<Ref<Image>> attachmentRefs;
  lib::Buffer<VkImageView> attachmentViews(3);
  {
    SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
    auto [colorAttachment, colorAttachmentMetadata] = createAttachment(
        *_logicalDevice, swapchainImageFormat, msaaSamples, extent, presentResources.numLayers,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    attachmentViews[0] = colorAttachment.getVkImageView();
    Ref<Image> collorAttachmentHandle =
        _imageManager->storeImage(std::move(colorAttachment), colorAttachmentMetadata);

    auto [depthAtachment, depthAtachmentMetadata] = createAttachment(
        *_logicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, msaaSamples, extent,
        presentResources.numLayers, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    attachmentViews[1] = depthAtachment.getVkImageView();
    Ref<Image> depthAttachmentHandle =
        _imageManager->storeImage(std::move(depthAtachment), depthAtachmentMetadata);

    const VkPhysicalDeviceFragmentShadingRatePropertiesKHR& fsrProperties =
        _physicalDevice->getFragmentShadingRateProperties();
    const VkExtent2D fsrTexelExtent = fsrProperties.maxFragmentShadingRateAttachmentTexelSize;
    const VkExtent2D fsrExtent = VkExtent2D{
      static_cast<uint32_t>(std::ceil(extent.width / static_cast<float>(fsrTexelExtent.width))),
      static_cast<uint32_t>(std::ceil(extent.height / static_cast<float>(fsrTexelExtent.height)))};
    auto [fsrTexture, fsrTextureMetadata] = createAttachment(
        *_logicalDevice, VK_FORMAT_R8_UINT, VK_SAMPLE_COUNT_1_BIT, fsrExtent,
        presentResources.numLayers, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_STORAGE_BIT);
    createFsrContents(*_logicalDevice, fsrTexture, fsrTextureMetadata, handle);

    attachmentViews[2] = fsrTexture.getVkImageView();
    _computeDescriptorSetWriter.storeImageStorage(
        fsrTexture.getVkImageView(), VK_IMAGE_LAYOUT_GENERAL);
    _computeDescriptorSetWriter.writeDescriptorSet(
        _logicalDevice->getVkDevice(), _computeDescriptorSet.getVkDescriptorSet());

    Ref<Image> fsrAttachmentHandle = _fsrTextureHandle =
        _imageManager->storeImage(std::move(fsrTexture), fsrTextureMetadata);

    attachmentRefs = lib::Buffer<Ref<Image>>{
      std::move(collorAttachmentHandle), std::move(depthAttachmentHandle),
      std::move(fsrAttachmentHandle)};
  }

  RenderpassBuilder renderpassBuilder(_attachmentLayout);
  if constexpr (MULTIVIEW_PRESENTATION) {
    auto mask = lib::setNLeastSignificantBits<uint32_t>(presentResources.numLayers);
    renderpassBuilder.withMultiView({mask}, {mask});
  }

  const VkExtent2D fsrTexelSize =
      _physicalDevice->getFragmentShadingRateProperties().maxFragmentShadingRateAttachmentTexelSize;
  renderpassBuilder.createSubpass()
      .addOutputAttachment(0)
      .addOutputAttachment(1)
      .addOutputAttachment(2)
      .withShadingRateAttachment(fsrTexelSize.width, fsrTexelSize.height);
  _renderPass =
      renderpassBuilder
          .addDependency(
              VK_SUBPASS_EXTERNAL, 0,
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                  | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                  | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
          .build(*_logicalDevice);

  auto imageViews = std::span<const VkImageView>(
      reinterpret_cast<const VkImageView*>(presentResources.imageViews.data()),
      presentResources.imageViews.size());
  for (VkImageView imageView : imageViews) {
    FramebufferBuilder framebufferBuilder;
    framebufferBuilder.addAttachment(imageView);
    for (VkImageView view : attachmentViews) {
      framebufferBuilder.addAttachment(view);
    }
    Framebuffer framebuffer = framebufferBuilder.build(_renderPass, extent, 1);
    _framebuffers.push_back(_framebufferAttachmentManager->storeFramebuffer(
        std::move(framebuffer), framebufferBuilder.getMetadata(), attachmentRefs, imageView));
  }
}

GCONTEXT_TEMPLATE
void GCONTEXT_CLASS waitDeviceIdle() const {
  vkDeviceWaitIdle(_logicalDevice->getVkDevice());
}

namespace {

std::tuple<Image, ImageMetadata> createSkybox(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    const AssetManager::ImageData& imageData, VkFormat format) {
  common::waitForAssetToLoad(imageData.loadState);
  auto [image, imageMetadata] =
      ImageBuilder()
          .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
          .withExtent(imageData.width, imageData.height)
          .withFormat(format)
          .withMipLevels(imageData.mipLevels)
          .withUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
          .withLayerCount(6)
          .withFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
          .buildImageWithMetadata(logicalDevice);
  commandBuffer.transitionImageLayout(
      image.getVkImage(), imageMetadata.imageAspect, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      imageMetadata.arrayLayers);
  Ref<Buffer> rfBuf = imageData.stagingBuffer;
  Ref<VirtualAllocation> rfVirt = imageData.virtualAllocation;
  commandBuffer.copyBufferToImage(rfBuf.getVkResource(), image.getVkImage(),
                                  internal::translateImageSubresourcesToVkBufferImageCopy(
                                      imageData.copyRegions, rfVirt.getMetadata().offset));
  commandBuffer.transitionImageLayout(
      image.getVkImage(), imageMetadata.imageAspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      imageMetadata.arrayLayers);
  ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, imageData.mipLevels, 0, 6);
  return std::make_tuple(std::move(image), imageMetadata);
}

std::tuple<Image, ImageMetadata> createCubemap(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    VkImageAspectFlags aspect, VkFormat format, VkImageUsageFlags additionalUsage) {
  auto [image, imageMetadata] =
      ImageBuilder()
          .withAspect(aspect)
          .withExtent(1024 * 4, 1024 * 4)
          .withFormat(format)
          .withUsage(VK_IMAGE_USAGE_SAMPLED_BIT | additionalUsage)
          .withLayerCount(6)
          .withFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
          .buildImageWithMetadata(logicalDevice);
  commandBuffer.transitionImageLayout(
      image.getVkImage(), imageMetadata.imageAspect, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      imageMetadata.arrayLayers);
  ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, 1, 0, 6);
  return std::make_tuple(std::move(image), imageMetadata);
}

std::tuple<Image, ImageMetadata> createShadowmap(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer, uint32_t width,
    uint32_t height, VkFormat format) {
  auto [image, imageMetadata] =
      ImageBuilder()
          .withAspect(VK_IMAGE_ASPECT_DEPTH_BIT)
          .withExtent(width, height)
          .withFormat(format)
          .withUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
          .buildImageWithMetadata(logicalDevice);
  commandBuffer.transitionImageLayout(
      image.getVkImage(), imageMetadata.imageAspect, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      imageMetadata.arrayLayers);
  ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, 1, 0, 1);
  return std::make_tuple(std::move(image), imageMetadata);
}

std::tuple<Image, ImageMetadata> createTexture2D(
    const LogicalDevice& logicalDevice, const CommandBuffer& commandBuffer,
    const AssetManager::ImageData& imageData, VkFormat format, float samplerAnisotropy) {
  auto [image, imageMetadata] =
      ImageBuilder()
          .withAspect(VK_IMAGE_ASPECT_COLOR_BIT)
          .withExtent(imageData.width, imageData.height)
          .withFormat(format)
          .withMipLevels(imageData.mipLevels)
          .withUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                     | VK_IMAGE_USAGE_SAMPLED_BIT)
          .buildImageWithMetadata(logicalDevice);
  commandBuffer.transitionImageLayout(
      image.getVkImage(), imageMetadata.imageAspect, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, imageMetadata.mipLevels, 0,
      imageMetadata.arrayLayers);
  Ref<Buffer> rfBuf = imageData.stagingBuffer;
  Ref<VirtualAllocation> rfVirt = imageData.virtualAllocation;
  commandBuffer.copyBufferToImage(rfBuf.getVkResource(), image.getVkImage(),
                                  internal::translateImageSubresourcesToVkBufferImageCopy(
                                      imageData.copyRegions, rfVirt.getMetadata().offset));
  commandBuffer.generateMipmaps(
      image.getVkImage(), imageMetadata.imageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      imageMetadata.imageExtent.width, imageMetadata.imageExtent.height, imageMetadata.mipLevels,
      imageMetadata.arrayLayers);
  ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, imageData.mipLevels, 0, 1);
  return std::make_tuple(std::move(image), imageMetadata);
}

std::tuple<Image, ImageMetadata> createAttachment(
    const LogicalDevice& logicalDevice, VkFormat format, VkSampleCountFlagBits samples,
    VkExtent2D extent, uint32_t numLayers, VkImageAspectFlags aspect, VkImageUsageFlags usage) {
  auto [image, imageMetadata] =
      ImageBuilder()
          .withFormat(format)
          .withNumSamples(samples)
          .withExtent(extent)
          .withLayerCount(numLayers)
          .withAspect(aspect)
          .withUsage(usage)
          .buildImageWithMetadata(logicalDevice);
  ImageViewBuilder().buildAndAddToImage(image, imageMetadata, 0, 1, 0, numLayers);
  return std::make_tuple(std::move(image), imageMetadata);
}

void createFsrContents(const LogicalDevice& logicalDevice, Image& image,
                       const ImageMetadata& metadata, const CommandBuffer& commandBuffer) {
  const lib::Buffer<std::byte> buffer(
      static_cast<size_t>(metadata.imageExtent.width * metadata.imageExtent.height), std::byte{10});
  auto [stagingBuffer, stagingMetadata] =
      BufferBuilder()
          .withSize(buffer.size())
          .withUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
          .buildStagingBufferWithMetadata(logicalDevice);
  common::copyData(stagingMetadata.getMappedMemoryAsSpan(), std::span(buffer));
  lib::Buffer<VkBufferImageCopy> imageCopy(metadata.arrayLayers);
  for (uint32_t layer = 0; layer < imageCopy.size(); layer++) {
    imageCopy[layer] = VkBufferImageCopy{
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel = 0,
                           .baseArrayLayer = layer,
                           .layerCount = 1},
      .imageExtent = metadata.imageExtent,
    };
  }

  commandBuffer.transitionImageLayout(
      image.getVkImage(), metadata.imageAspect, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, metadata.mipLevels, 0, metadata.arrayLayers);
  commandBuffer.copyBufferToImage(stagingBuffer.getVkBuffer(), image.getVkImage(), imageCopy);
  commandBuffer.transitionImageLayout(
      image.getVkImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, 0, metadata.mipLevels, 0, metadata.arrayLayers);
}

}  // namespace

// Explicit instantiation of the specializations that are actually used.
// These must appear AFTER all member definitions above.
template class GraphicsContext<false, false>;

template class GraphicsContext<true, true>;

}  // namespace vlkn

#undef GCONTEXT_CLASS
#undef GCONTEXT_TEMPLATE
