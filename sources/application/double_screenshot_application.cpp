#include "double_screenshot_application.h"

#include "lib/status/status.h"
#include "memory_objects/texture/texture_factory.h"
#include "model_loader/tiny_gltf_loader/tiny_gltf_loader.h"
#include "entity_component_system/system/movement_system.h"
#include "entity_component_system/component/material.h"
#include "entity_component_system/component/mesh.h"
#include "entity_component_system/component/position.h"
#include "entity_component_system/component/transform.h"
#include "entity_component_system/component/velocity.h"
#include "pipeline/shader/shader_program.h"
#include "primitives/vertex_builder.h"
#include "render_pass/attachment/attachment_layout.h"
#include "thread_pool/thread_pool.h"
#include "memory_objects/staging_buffer.h"

#include <algorithm>
#include <array>
#include <chrono>


SingleApp::SingleApp()
    : ApplicationBase() {
    _assetManager = std::make_unique<AssetManager>(_logicalDevice->getMemoryAllocator());

    createDescriptorSets();
    loadObjects();
    loadObject();
    loadCubemap();
    createPresentResources();
    createShadowResources();

    createCommandBuffers();
    //_screenshot = std::make_unique<Screenshot>(*_logicalDevice);
    //_screenshot->addImageToObserved(_framebufferTextures[1]->getImage(), "hig_res_screenshot.ppm");
    _camera = std::make_unique<FPSCamera>(glm::radians(45.0f), 1920.0f / 1080.0f, 0.01f, 100.0f);

    _callbackManager = std::make_unique<FPSCallbackManager>(_window.get());
    _callbackManager->attach(_camera.get());
    //_callbackManager->attach(_screenshot.get());

    createSyncObjects();
}

lib::Status SingleApp::loadCubemap() {
    ASSIGN_OR_RETURN(const VertexData vertexDataCube, loadObj(MODELS_PATH "cube.obj"));
    ASSIGN_OR_RETURN(const lib::Buffer<VertexP> vertices, buildInterleavingVertexData(vertexDataCube.positions));
    RETURN_IF_ERROR(_assetManager->loadVertexData("cube.obj", vertices, vertexDataCube.indices, static_cast<uint8_t>(vertexDataCube.indexType)));
    {
        SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
        const VkCommandBuffer commandBuffer = handle.getCommandBuffer();
        const AssetManager::VertexData& vData = _assetManager->getVertexData("cube.obj");
        ASSIGN_OR_RETURN(_vertexBufferCube, VertexBuffer::create(*_logicalDevice, commandBuffer, vData.vertexBufferPrimitives));
        ASSIGN_OR_RETURN(_indexBufferCube, IndexBuffer::create(*_logicalDevice, commandBuffer, vData.indexBuffer, vData.indexType));
    }
    return lib::StatusOk();
}

lib::Status SingleApp::loadObject() {
    const std::string drakanTexturePath = TEXTURES_PATH "drakan.jpg";
    _assetManager->loadImage2DAsync(drakanTexturePath);

    const auto& propertyManager = _physicalDevice->getPropertyManager();
    float maxSamplerAnisotropy = propertyManager.getMaxSamplerAnisotropy();

    {
        SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
        const VkCommandBuffer commandBuffer = handle.getCommandBuffer();

        ASSIGN_OR_RETURN(const VertexData vertexData, loadObj(MODELS_PATH "cylinder8.obj"));
        ASSIGN_OR_RETURN(const lib::Buffer<VertexPTN> vertices, buildInterleavingVertexData(vertexData.positions, vertexData.textureCoordinates, vertexData.normals));
        RETURN_IF_ERROR(_assetManager->loadVertexData("cube_normal.obj", vertices, vertexData.indices, static_cast<uint8_t>(vertexData.indexType)));
        const AssetManager::VertexData& vData = _assetManager->getVertexData("cube_normal.obj");
        ASSIGN_OR_RETURN(_vertexBufferObject, VertexBuffer::create(*_logicalDevice, commandBuffer, *vData.vertexBuffer));
        ASSIGN_OR_RETURN(_vertexBufferPrimitiveObject, VertexBuffer::create(*_logicalDevice, commandBuffer, vData.vertexBufferPrimitives));
        ASSIGN_OR_RETURN(_indexBufferObject, IndexBuffer::create(*_logicalDevice, commandBuffer, vData.indexBuffer, vData.indexType));

        ASSIGN_OR_RETURN(const AssetManager::ImageData* imgData, _assetManager->getImageData(drakanTexturePath));
        ASSIGN_OR_RETURN(auto texture, TextureFactory::create2DTextureImage(*_logicalDevice, commandBuffer, imgData->stagingBuffer, imgData->imageDimensions, VK_FORMAT_R8G8B8A8_SRGB, maxSamplerAnisotropy));
        _textures.emplace_back(std::move(texture));
    }

    _uniformMap.emplace(drakanTexturePath, std::make_shared<UniformBufferTexture>(*_textures.back()));
    UniformBufferObject object = {
        .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f))
    };
    _objectUniform = std::make_unique<UniformBufferData<UniformBufferObject>>(*_logicalDevice);
    _objectUniform->updateUniformBuffer(object);
    ASSIGN_OR_RETURN(auto descriptorSet, _descriptorPoolNormal->createDesriptorSet());
    descriptorSet->updateDescriptorSet({ _dynamicUniformBuffersCamera.get(), _uniformBuffersLight.get(), _objectUniform.get(), _uniformMap[drakanTexturePath].get(), _shadowTextureUniform.get() });
    _objectEntity = _registry.createEntity();
    _entitytoDescriptorSet.emplace(_objectEntity, std::move(descriptorSet));

    return lib::StatusOk();
}

lib::Status SingleApp::loadObjects() {
    // TODO needs refactoring
    ASSIGN_OR_RETURN(auto sceneData, LoadGltf(MODELS_PATH "sponza/scene.gltf"));
    for (uint32_t i = 0; i < sceneData.size(); i++) {
        if (sceneData[i].normalTexture.empty() || sceneData[i].metallicRoughnessTexture.empty())
            continue;
        _assetManager->loadImage2DAsync(MODELS_PATH "sponza/" + sceneData[i].diffuseTexture);
        _assetManager->loadImage2DAsync(MODELS_PATH "sponza/" + sceneData[i].metallicRoughnessTexture);
        _assetManager->loadImage2DAsync(MODELS_PATH "sponza/" + sceneData[i].normalTexture);
        const auto vertices = buildInterleavingVertexData(sceneData[i].positions, sceneData[i].textureCoordinates, sceneData[i].normals, sceneData[i].tangents);
        if (vertices.has_value())
            _assetManager->loadVertexData(std::to_string(i), *vertices, sceneData[i].indices, static_cast<uint8_t>(sceneData[i].indexType));
    }
    const auto& propertyManager = _physicalDevice->getPropertyManager();
    float maxSamplerAnisotropy = propertyManager.getMaxSamplerAnisotropy();
    _objects.reserve(sceneData.size());
    {
        SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
        const VkCommandBuffer commandBuffer = handle.getCommandBuffer();
        for (uint32_t i = 0; i < sceneData.size(); i++) {
            Entity e = _registry.createEntity();
            if (sceneData[i].normalTexture.empty() || sceneData[i].metallicRoughnessTexture.empty())
                continue;
            const std::string diffusePath = std::string(MODELS_PATH) + "sponza/" + sceneData[i].diffuseTexture;
            const std::string metallicRoughnessPath = std::string(MODELS_PATH) + "sponza/" + sceneData[i].metallicRoughnessTexture;
            const std::string normalPath = std::string(MODELS_PATH) + "sponza/" + sceneData[i].normalTexture;

            if (!_uniformMap.contains(diffusePath)) {
                ASSIGN_OR_RETURN(const AssetManager::ImageData* imgData, _assetManager->getImageData(diffusePath));
                ASSIGN_OR_RETURN(auto texture, TextureFactory::create2DTextureImage(*_logicalDevice, commandBuffer, imgData->stagingBuffer, imgData->imageDimensions, VK_FORMAT_R8G8B8A8_SRGB, maxSamplerAnisotropy));
                _textures.emplace_back(std::move(texture));
                _uniformMap.emplace(diffusePath, std::make_shared<UniformBufferTexture>(*_textures.back()));
            }
            if (!_uniformMap.contains(normalPath)) {
                ASSIGN_OR_RETURN(const AssetManager::ImageData* imgData, _assetManager->getImageData(normalPath));
                ASSIGN_OR_RETURN(auto texture, TextureFactory::create2DTextureImage(*_logicalDevice, commandBuffer, imgData->stagingBuffer, imgData->imageDimensions, VK_FORMAT_R8G8B8A8_UNORM, maxSamplerAnisotropy));
                _textures.emplace_back(std::move(texture));
                _uniformMap.emplace(normalPath, std::make_shared<UniformBufferTexture>(*_textures.back()));
            }
            if (!_uniformMap.contains(metallicRoughnessPath)) {
                ASSIGN_OR_RETURN(const AssetManager::ImageData* imgData, _assetManager->getImageData(metallicRoughnessPath));
                ASSIGN_OR_RETURN(auto texture, TextureFactory::create2DTextureImage(*_logicalDevice, commandBuffer, imgData->stagingBuffer, imgData->imageDimensions, VK_FORMAT_R8G8B8A8_UNORM, maxSamplerAnisotropy));
                _textures.emplace_back(std::move(texture));
                _uniformMap.emplace(metallicRoughnessPath, std::make_shared<UniformBufferTexture>(*_textures.back()));
            }

            ASSIGN_OR_RETURN(auto descriptorSet, _descriptorPool->createDesriptorSet());
            descriptorSet->updateDescriptorSet({ _dynamicUniformBuffersCamera.get(), _uniformBuffersLight.get(), _uniformBuffersObjects.get(), _uniformMap[diffusePath].get(), _shadowTextureUniform.get(), _uniformMap[normalPath].get(), _uniformMap[metallicRoughnessPath].get() });;

            _objects.emplace_back("Object", e);
            const AssetManager::VertexData& vData = _assetManager->getVertexData(std::to_string(i));
            MeshComponent msh;
            msh.vertexBuffer = VertexBuffer::create(*_logicalDevice, commandBuffer, vData.vertexBuffer.value()).value();
            msh.indexBuffer = IndexBuffer::create(*_logicalDevice, commandBuffer, vData.indexBuffer, vData.indexType).value();
            msh.vertexBufferPrimitive = VertexBuffer::create(*_logicalDevice, commandBuffer, vData.vertexBufferPrimitives).value();
            msh.aabb = createAABBfromVertices(std::vector<glm::vec3>(sceneData[i].positions.cbegin(), sceneData[i].positions.cend()), sceneData[i].model);
            _registry.addComponent<MeshComponent>(e, std::move(msh));

            TransformComponent trsf;
            trsf.model = sceneData[i].model;
            _registry.addComponent<TransformComponent>(e, std::move(trsf));

            _entityToIndex.emplace(e, index);
            _entitytoDescriptorSet.emplace(e, std::move(descriptorSet));
        
            _ubObject.model = sceneData[i].model;
            _uniformBuffersObjects->updateUniformBuffer(_ubObject, index++);
        }
        _ubObject.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
        _uniformBuffersObjects->updateUniformBuffer(_ubObject, index);
    }
    AABB sceneAABB = _registry.getComponent<MeshComponent>(_objects[0].getEntity()).aabb;
    for (int i = 1; i < _objects.size(); i++) {
        sceneAABB.extend(_registry.getComponent<MeshComponent>(_objects[i].getEntity()).aabb);
    }
    _octree = std::make_unique<Octree>(sceneAABB);

    for (const auto& object : _objects)
        _octree->addObject(&object, _registry.getComponent<MeshComponent>(object.getEntity()).aabb);

    return lib::StatusOk();
}

lib::Status SingleApp::createDescriptorSets() {
    const auto& propertyManager = _physicalDevice->getPropertyManager();
    float maxSamplerAnisotropy = propertyManager.getMaxSamplerAnisotropy();
    _assetManager->loadImageCubemapAsync(TEXTURES_PATH "cubemap_yokohama_rgba.ktx");
    {
        SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
        const VkCommandBuffer commandBuffer = handle.getCommandBuffer();

        ASSIGN_OR_RETURN(const AssetManager::ImageData* imgData, _assetManager->getImageData(TEXTURES_PATH "cubemap_yokohama_rgba.ktx"));
        ASSIGN_OR_RETURN(_textureCubemap, TextureFactory::createTextureCubemap(*_logicalDevice, commandBuffer, imgData->stagingBuffer, imgData->imageDimensions, VK_FORMAT_R8G8B8A8_UNORM, maxSamplerAnisotropy));
        ASSIGN_OR_RETURN(_shadowMap, TextureFactory::create2DShadowmap(*_logicalDevice, commandBuffer, 1024 * 2, 1024 * 2, VK_FORMAT_D32_SFLOAT));
    }

    _uniformBuffersObjects = std::make_unique<UniformBufferData<UniformBufferObject>>(*_logicalDevice, 200);
    _uniformBuffersLight = std::make_unique<UniformBufferData<UniformBufferLight>>(*_logicalDevice);
    _dynamicUniformBuffersCamera = std::make_unique<UniformBufferData<UniformBufferCamera>>(*_logicalDevice, MAX_FRAMES_IN_FLIGHT);

    _skyboxTextureUniform = std::make_unique<UniformBufferTexture>(*_textureCubemap);
    _shadowTextureUniform = std::make_unique<UniformBufferTexture>(*_shadowMap);

    _pbrShaderProgram = ShaderProgramFactory::createShaderProgram(ShaderProgramType::PBR_TESSELLATION, *_logicalDevice);
    _normalShaderProgram = ShaderProgramFactory::createShaderProgram(ShaderProgramType::NORMAL, *_logicalDevice);
    _skyboxShaderProgram = ShaderProgramFactory::createShaderProgram(ShaderProgramType::SKYBOX, *_logicalDevice);
    _shadowShaderProgram = ShaderProgramFactory::createShaderProgram(ShaderProgramType::SHADOW, *_logicalDevice);

    ASSIGN_OR_RETURN(_descriptorPool, DescriptorPool::create(*_logicalDevice, _pbrShaderProgram->getDescriptorSetLayout(), 150));
    ASSIGN_OR_RETURN(_descriptorPoolNormal, DescriptorPool::create(*_logicalDevice, _normalShaderProgram->getDescriptorSetLayout(), 1));
    ASSIGN_OR_RETURN(_descriptorPoolSkybox, DescriptorPool::create(*_logicalDevice, _skyboxShaderProgram->getDescriptorSetLayout(), 1));
    ASSIGN_OR_RETURN(_descriptorPoolShadow, DescriptorPool::create(*_logicalDevice, _shadowShaderProgram->getDescriptorSetLayout(), 2));

    ASSIGN_OR_RETURN(_descriptorSetSkybox, _descriptorPoolSkybox->createDesriptorSet());
    ASSIGN_OR_RETURN(_descriptorSetShadow, _descriptorPoolShadow->createDesriptorSet());

    _descriptorSetSkybox->updateDescriptorSet({ _dynamicUniformBuffersCamera.get(), _skyboxTextureUniform.get() });
    _descriptorSetShadow->updateDescriptorSet({ _uniformBuffersLight.get(),  _uniformBuffersObjects.get() });

    _ubLight.pos = glm::vec3(15.1891f, 2.66408f, -0.841221f);
    _ubLight.projView = glm::perspective(glm::radians(120.0f), 1.0f, 0.01f, 40.0f);
    _ubLight.projView[1][1] = -_ubLight.projView[1][1];
    _ubLight.projView = _ubLight.projView * glm::lookAt(_ubLight.pos, glm::vec3(-3.82383f, 3.66503f, 1.30751f), glm::vec3(0.0f, 1.0f, 0.0f));
    _uniformBuffersLight->updateUniformBuffer(_ubLight);

    return lib::StatusOk();
}

void SingleApp::createPresentResources() {
    const VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_4_BIT;
    const VkFormat swapchainImageFormat = _swapchain->getVkFormat();
    const VkExtent2D extent = _swapchain->getExtent();

    AttachmentLayout attachmentsLayout(msaaSamples);
    attachmentsLayout.addColorResolvePresentAttachment(swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
    //              .addColorResolveAttachment(swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE)
                    .addColorAttachment(swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
    //              .addColorAttachment(swapchainImageFormat, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
                    .addDepthAttachment(findDepthFormat(), VK_ATTACHMENT_STORE_OP_DONT_CARE);

    _renderPass = Renderpass::create(*_logicalDevice, attachmentsLayout).value();
    _renderPass->addSubpass({0, 1, 2});
    _renderPass->addDependency(VK_SUBPASS_EXTERNAL,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    );
    _renderPass->build();

    _framebuffers.reserve(_swapchain->getImagesCount());
    for (uint8_t i = 0; i < _swapchain->getImagesCount(); ++i) {
        _framebuffers.emplace_back(Framebuffer::createFromSwapchain(*_renderPass, *_swapchain, *_singleTimeCommandPool, i).value());
    }
    {
        const GraphicsPipelineParameters parameters = {
            .msaaSamples = msaaSamples,
            .patchControlPoints = 3,
        };
        _graphicsPipeline = std::make_unique<GraphicsPipeline>(*_renderPass, *_pbrShaderProgram, parameters);
    }
    {
        const GraphicsPipelineParameters parameters = {
            .cullMode = VK_CULL_MODE_FRONT_BIT,
            .msaaSamples = msaaSamples
        };
        _graphicsPipelineSkybox = std::make_unique<GraphicsPipeline>(*_renderPass, *_skyboxShaderProgram, parameters);
    }
    {
        const GraphicsPipelineParameters parameters = {
            .msaaSamples = msaaSamples,
            .patchControlPoints = 3
        };
        _graphicsPipelineNormal = std::make_unique<GraphicsPipeline>(*_renderPass, *_normalShaderProgram, parameters);
    }
}

void SingleApp::createShadowResources() {
    const VkFormat imageFormat = VK_FORMAT_D32_SFLOAT;
    const VkExtent2D extent = { 1024 * 2, 1024 * 2 };
    AttachmentLayout attachmentLayout;
    attachmentLayout.addShadowAttachment(imageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _shadowRenderPass = Renderpass::create(*_logicalDevice, attachmentLayout).value();
    _shadowRenderPass->addSubpass({0});
    _shadowRenderPass->build();

    _shadowFramebuffer = Framebuffer::createFromTextures(*_shadowRenderPass, { _shadowMap }).value();

    const GraphicsPipelineParameters parameters = {
        .cullMode = VK_CULL_MODE_FRONT_BIT,
        .depthBiasConstantFactor = 0.7f,
        .depthBiasSlopeFactor = 2.0f,
    };
    _shadowPipeline = std::make_unique<GraphicsPipeline>(*_shadowRenderPass, *_shadowShaderProgram, parameters);
}

SingleApp::~SingleApp() {
    const VkDevice device = _logicalDevice->getVkDevice();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, _inFlightFences[i], nullptr);
    }
}

SingleApp& SingleApp::getInstance() {
    static SingleApp application;
    return application;
}

void SingleApp::run() {
    updateUniformBuffer(_currentFrame);
    {
        SingleTimeCommandBuffer handle(*_singleTimeCommandPool);
        VkCommandBuffer commandBuffer = handle.getCommandBuffer();
        recordShadowCommandBuffer(commandBuffer, 0);
    }

    for (auto& object : _objects) {
        _registry.getComponent<MeshComponent>(object.getEntity()).vertexBufferPrimitive.reset();
    }

    while (_window->open()) {
        _callbackManager->pollEvents();
        draw();
    }
    vkDeviceWaitIdle(_logicalDevice->getVkDevice());
}

void SingleApp::draw() {
    VkDevice device = _logicalDevice->getVkDevice();
    vkWaitForFences(device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = _swapchain->acquireNextImage(_imageAvailableSemaphores[_currentFrame], &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(_currentFrame);
    vkResetFences(device, 1, &_inFlightFences[_currentFrame]);

    _primaryCommandBuffer[_currentFrame]->resetCommandBuffer();
    for(int i = 0; i < MAX_THREADS_IN_POOL; i++)
        _commandBuffers[_currentFrame][i]->resetCommandBuffer();

    //recordShadowCommandBuffer(_shadowCommandBuffers[_currentFrame], imageIndex);
    recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    std::array<VkCommandBuffer, 1> submitCommands = { _primaryCommandBuffer[_currentFrame]->getVkCommandBuffer() };
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommands.size());
    submitInfo.pCommandBuffers = submitCommands.data();

    VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_logicalDevice->getGraphicsQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    result = _swapchain->present(imageIndex, signalSemaphores[0]);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    if (++_currentFrame == MAX_FRAMES_IN_FLIGHT) {
        _currentFrame = 0;
    }
}

VkFormat SingleApp::findDepthFormat() const {
    const std::array<VkFormat, 3> depthFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    auto formatPtr = std::find_if(depthFormats.cbegin(), depthFormats.cend(), [&](VkFormat format) {
        return _physicalDevice->getPropertyManager().checkTextureFormatSupport(format, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        });

    if (formatPtr == depthFormats.cend()) {
        throw std::runtime_error("failed to find depth format!");
    }

    return *formatPtr;
}

void SingleApp::createSyncObjects() {
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    const VkDevice device = _logicalDevice->getVkDevice();

    const VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    const VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void SingleApp::updateUniformBuffer(uint32_t currentFrame) {
    _ubCamera.view = _camera->getViewMatrix();
    _ubCamera.proj = _camera->getProjectionMatrix();
    _ubCamera.pos = _camera->getPosition();
    _dynamicUniformBuffersCamera->updateUniformBuffer(_ubCamera, currentFrame);
}

void SingleApp::createCommandBuffers() {
    _commandPool.reserve(MAX_THREADS_IN_POOL + 1);
    for (int i = 0; i < MAX_THREADS_IN_POOL + 1; i++) {
        _commandPool.emplace_back(std::make_unique<CommandPool>(*_logicalDevice));
    }

    _primaryCommandBuffer.reserve(MAX_FRAMES_IN_FLIGHT);
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _shadowCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _primaryCommandBuffer.emplace_back(_commandPool[MAX_THREADS_IN_POOL]->createPrimaryCommandBuffer());
        _commandBuffers[i].reserve(MAX_THREADS_IN_POOL);
        _shadowCommandBuffers[i].reserve(MAX_THREADS_IN_POOL);
        for (int j = 0; j < MAX_THREADS_IN_POOL; j++) {
            _commandBuffers[i].emplace_back(_commandPool[j]->createSecondaryCommandBuffer());
            _shadowCommandBuffers[i].emplace_back(_commandPool[j]->createSecondaryCommandBuffer());
        }
    }
}

void SingleApp::recordOctreeSecondaryCommandBuffer(const VkCommandBuffer commandBuffer, const OctreeNode* rootNode, const std::array<glm::vec4, NUM_CUBE_FACES>& planes) {
    if (!rootNode || !rootNode->getVolume().intersectsFrustum(planes)) return;

    static std::queue<const OctreeNode*> nodeQueue;
    nodeQueue.push(rootNode);

    static constexpr OctreeNode::Subvolume options[] = {
        OctreeNode::Subvolume::LOWER_LEFT_BACK, OctreeNode::Subvolume::LOWER_LEFT_FRONT,
        OctreeNode::Subvolume::LOWER_RIGHT_BACK, OctreeNode::Subvolume::LOWER_RIGHT_FRONT,
        OctreeNode::Subvolume::UPPER_LEFT_BACK, OctreeNode::Subvolume::UPPER_LEFT_FRONT,
        OctreeNode::Subvolume::UPPER_RIGHT_BACK, OctreeNode::Subvolume::UPPER_RIGHT_FRONT
    };

    while (!nodeQueue.empty()) {
        const OctreeNode* node = nodeQueue.front();
        nodeQueue.pop();

        for (const Object* object : node->getObjects()) {
            const auto& meshComponent = _registry.getComponent<MeshComponent>(object->getEntity());
            const IndexBuffer& indexBuffer = *meshComponent.indexBuffer;
            const VertexBuffer& vertexBuffer = *meshComponent.vertexBuffer;
            vertexBuffer.bind(commandBuffer);
            indexBuffer.bind(commandBuffer);
            _entitytoDescriptorSet[object->getEntity()]->bind(commandBuffer, *_graphicsPipeline, { _currentFrame, _entityToIndex[object->getEntity()] });
            vkCmdDrawIndexed(commandBuffer, indexBuffer.getIndexCount(), 1, 0, 0, 0);
        }

        for (auto option : options) {
            const OctreeNode* childNode = node->getChild(option);
            if (childNode && childNode->getVolume().intersectsFrustum(planes)) {
                nodeQueue.push(childNode);
            }
        }
    }
}

void SingleApp::recordCommandBuffer(uint32_t imageIndex) {
    const Framebuffer& framebuffer = *_framebuffers[imageIndex];
    const PrimaryCommandBuffer& primaryCommandBuffer = *_primaryCommandBuffer[_currentFrame];
    primaryCommandBuffer.begin();
    primaryCommandBuffer.beginRenderPass(framebuffer);

    //const VkExtent2D framebufferExtent = framebuffer.getVkExtent();
    //const VkViewport viewport = {
    //    .x = 0.0f,
    //    .y = 0.0f,
    //    .width = (float)framebufferExtent.width,
    //    .height = (float)framebufferExtent.height,
    //    .minDepth = 0.0f,
    //    .maxDepth = 1.0f
    //};
    //const VkRect2D scissor = {
    //    .offset = { 0, 0 },
    //    .extent = framebufferExtent
    //};

    const VkCommandBufferInheritanceViewportScissorInfoNV scissorViewportInheritance = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV,
        .viewportScissor2D = VK_TRUE,
        .viewportDepthCount = 1,
        .pViewportDepths = &framebuffer.getViewport(),
    };

    std::array<std::future<void>, MAX_THREADS_IN_POOL> futures;

    futures[0] = std::async(std::launch::async, [&]() {
        _commandBuffers[_currentFrame][0]->begin(framebuffer, &scissorViewportInheritance);
        const VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame][0]->getVkCommandBuffer();
        vkCmdBindPipeline(commandBuffer, _graphicsPipeline->getVkPipelineBindPoint(), _graphicsPipeline->getVkPipeline());

        const OctreeNode* root = _octree->getRoot();
        const auto& planes = extractFrustumPlanes(_camera->getProjectionMatrix() * _camera->getViewMatrix());
        recordOctreeSecondaryCommandBuffer(commandBuffer, root, planes);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
        });

    futures[1] = std::async(std::launch::async, [&]() {
        // Skybox
        _commandBuffers[_currentFrame][1]->begin(framebuffer, &scissorViewportInheritance);
        const VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame][1]->getVkCommandBuffer();

        vkCmdBindPipeline(commandBuffer, _graphicsPipelineSkybox->getVkPipelineBindPoint(), _graphicsPipelineSkybox->getVkPipeline());
        _vertexBufferCube->bind(commandBuffer);
        _indexBufferCube->bind(commandBuffer);
        _descriptorSetSkybox->bind(commandBuffer, *_graphicsPipelineSkybox, { _currentFrame });
        vkCmdDrawIndexed(commandBuffer, _indexBufferCube->getIndexCount(), 1, 0, 0, 0);

        // Object
        vkCmdBindPipeline(commandBuffer, _graphicsPipelineNormal->getVkPipelineBindPoint(), _graphicsPipelineNormal->getVkPipeline());
        _vertexBufferObject->bind(commandBuffer);
        _indexBufferObject->bind(commandBuffer);
        _entitytoDescriptorSet[_objectEntity]->bind(commandBuffer, *_graphicsPipelineNormal, { _currentFrame });
        vkCmdDrawIndexed(commandBuffer, _indexBufferObject->getIndexCount(), 1, 0, 0, 0);

        vkEndCommandBuffer(commandBuffer);
        });

    futures[0].wait();
    futures[1].wait();

    primaryCommandBuffer.executeSecondaryCommandBuffers({ _commandBuffers[_currentFrame][0]->getVkCommandBuffer(), _commandBuffers[_currentFrame][1]->getVkCommandBuffer() });
    primaryCommandBuffer.endRenderPass();

    if (primaryCommandBuffer.end() != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void SingleApp::recordShadowCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    //if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to begin recording command buffer!");
    //}

    VkExtent2D extent = _shadowMap->getVkExtent2D();
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _shadowRenderPass->getVkRenderPass();
    renderPassInfo.framebuffer = _shadowFramebuffer->getVkFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    const auto& clearValues = _shadowRenderPass->getAttachmentsLayout().getVkClearValues();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindPipeline(commandBuffer, _shadowPipeline->getVkPipelineBindPoint(), _shadowPipeline->getVkPipeline());

    for (const auto& object : _objects) {
        const auto& meshComponent = _registry.getComponent<MeshComponent>(object.getEntity());
        VkBuffer vertexBuffers[] = { meshComponent.vertexBufferPrimitive->getVkBuffer() };
        const IndexBuffer* indexBuffer = meshComponent.indexBuffer.get();
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        _descriptorSetShadow->bind(commandBuffer, *_shadowPipeline, { _entityToIndex[object.getEntity()] });
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getVkBuffer(), 0, indexBuffer->getIndexType());
        vkCmdDrawIndexed(commandBuffer, indexBuffer->getIndexCount(), 1, 0, 0, 0);
    }

    _vertexBufferPrimitiveObject->bind(commandBuffer);
    _indexBufferObject->bind(commandBuffer);
    _descriptorSetShadow->bind(commandBuffer, *_shadowPipeline, { index });
    vkCmdDrawIndexed(commandBuffer, _indexBufferObject->getIndexCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    //if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to record command buffer!");
    //}
}

void SingleApp::recreateSwapChain() {
    VkExtent2D extent{};
    while (extent.width == 0 || extent.height == 0) {
        extent = _window->getFramebufferSize();
        //glfwWaitEvents();
    }

    _camera->setAspectRatio(static_cast<float>(extent.width) / extent.height);
    vkDeviceWaitIdle(_logicalDevice->getVkDevice());

    _swapchain = Swapchain::create(*_logicalDevice, _swapchain->getVkSwapchain()).value();
    for (uint8_t i = 0; i < _swapchain->getImagesCount(); ++i) {
        _framebuffers[i] = Framebuffer::createFromSwapchain(*_renderPass, *_swapchain, *_singleTimeCommandPool, i).value();
    }
}