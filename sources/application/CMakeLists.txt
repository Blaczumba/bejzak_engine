# add_library(Application application_base.cpp offscreen_rendering_application.cpp)
add_library(Application application_base.cpp double_screenshot_application.cpp)

target_link_libraries(Application PRIVATE Vulkan::Vulkan)
if(Boost_FOUND)
	target_link_libraries(Application PUBLIC ${Boost_LIBRARIES})
endif()

target_link_libraries(Application PRIVATE LibStrongTypes)
target_link_libraries(Application PUBLIC Window Instance Surface DebugMessenger PhysicalDevice MemoryAllocator LogicalDevice Renderpass Attachment Swapchain Buffer CommandBuffer Framebuffer UniformBuffer Pipeline Texture  OBJLoader TinyGLTFLoader DescriptorSet Camera Screenshot Entity ComponentSystem ECSRegistry Object ThreadPool Primitives AssetManager)
target_link_libraries(Application PRIVATE OBJLoader)

target_include_directories(Application PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})