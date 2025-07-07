#pragma once

#include "application_base.h"

#include "camera/fps_camera.h"
#include "command_buffer/command_buffer.h"
#include "descriptor_set/bindless_descriptor_set_writer.h"
#include "descriptor_set/descriptor_pool.h"
#include "descriptor_set/descriptor_set.h"
#include "descriptor_set/descriptor_set_layout.h"
#include "descriptor_set/descriptor_set_writer.h"
#include "entity_component_system/system/movement_system.h"
#include "memory_objects/buffer.h"
#include "memory_objects/texture/texture.h"
#include "memory_objects/uniform_buffer/push_constants.h"
#include "memory_objects/uniform_buffer/uniform_buffer.h"
#include "model_loader/obj_loader/obj_loader.h"
#include "object/object.h"
#include "framebuffer/framebuffer.h"
#include "render_pass/render_pass.h"
#include "resource_manager/asset_manager.h"
#include "resource_manager/resource_manager.h"
#include "scene/octree.h"
#include "screenshot/screenshot.h"
#include "status/status.h"
#include "thread_pool/thread_pool.h"
#include "pipeline/graphics_pipeline.h"

#include <unordered_map>

class SingleApp : public ApplicationBase {
    uint32_t index = 0;
    std::unordered_map<std::string, std::pair<TextureHandle, std::unique_ptr<Texture>>> _textures;
    std::unordered_map<std::string, Buffer> _vertexBufferMap;
    std::unordered_map<std::string, Buffer> _indexBufferMap;
    std::unordered_map<Entity, uint32_t> _entityToIndex;
    std::vector<Object> _objects;
    std::unique_ptr<Octree> _octree;
    Registry _registry;
    std::unique_ptr<AssetManager> _assetManager;
    std::unique_ptr<ResourceManager> _resourceManager;
    std::shared_ptr<Renderpass> _renderPass;
    std::vector<std::unique_ptr<Framebuffer>> _framebuffers;

    // Shadowmap
    std::shared_ptr<Renderpass> _shadowRenderPass;
    std::unique_ptr<Framebuffer> _shadowFramebuffer;
    std::shared_ptr<Texture> _shadowMap;
    std::unique_ptr<GraphicsPipeline> _shadowPipeline;
    TextureHandle _shadowHandle;

    // Cubemap.
    Buffer _vertexBufferCube;
    Buffer _indexBufferCube;
    std::unique_ptr<Texture> _textureCubemap;
    VkIndexType _indexBufferCubeType;
    std::unique_ptr<GraphicsPipeline> _graphicsPipelineSkybox;
    std::shared_ptr<DescriptorPool> _descriptorPoolSkybox;
    std::unique_ptr<GraphicsShaderProgram> _skyboxShaderProgram;
    TextureHandle _skyboxHandle;

    // PBR objects.
    std::vector<Object> objects;
    std::shared_ptr<DescriptorPool> _descriptorPool;
    std::shared_ptr<DescriptorPool> _dynamicDescriptorPool;
    std::unique_ptr<GraphicsShaderProgram> _pbrShaderProgram;
    std::unique_ptr<GraphicsPipeline> _graphicsPipeline;
    UniformBufferCamera _ubCamera;
    UniformBufferLight _ubLight;

    DescriptorSetWriter _dynamicDescriptorSetWriter;
    Buffer _dynamicUniformBuffersCamera;
    DescriptorSet _dynamicDescriptorSet;

    std::unique_ptr<GraphicsShaderProgram> _shadowShaderProgram;
    std::unique_ptr<BindlessDescriptorSetWriter> _bindlessWriter;

    DescriptorSet _bindlessDescriptorSet;
    Buffer _lightBuffer;
    BufferHandle _lightHandle;

    std::unique_ptr<FPSCamera> _camera;

    std::vector<std::unique_ptr<CommandPool>> _commandPool;
    std::vector<std::unique_ptr<PrimaryCommandBuffer>> _primaryCommandBuffer;
    std::vector<std::vector<std::unique_ptr<SecondaryCommandBuffer>>> _commandBuffers;
    std::vector<std::vector<std::unique_ptr<SecondaryCommandBuffer>>> _shadowCommandBuffers;

    std::vector<VkSemaphore> _shadowMapSemaphores;
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    static constexpr uint32_t MAX_THREADS_IN_POOL = 2;

    float _mouseXOffset;
    float _mouseYOffset;

public:
    SingleApp();
    ~SingleApp();

    SingleApp(const SingleApp&) = delete;
    SingleApp(SingleApp&&) = delete;
    void operator=(const SingleApp&) = delete;

    void run() override;
private:
    void setInput();
    void draw();
    VkFormat findDepthFormat() const;
    void createCommandBuffers();
    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(uint32_t imageIndex);
    void recordOctreeSecondaryCommandBuffer(const VkCommandBuffer commandBuffer, const OctreeNode* node, const std::array<glm::vec4, NUM_CUBE_FACES>& planes);
    void recordShadowCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    Status recreateSwapChain();

    Status createDescriptorSets();
    Status createPresentResources();
    Status createShadowResources();

    Status loadObjects();
    Status loadObject();
    Status loadCubemap();
};
