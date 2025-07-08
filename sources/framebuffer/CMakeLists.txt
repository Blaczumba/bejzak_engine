add_library(Framebuffer framebuffer.cpp)

target_link_libraries(Framebuffer PUBLIC LogicalDevice Renderpass Swapchain Buffer CommandBuffer)
target_link_libraries(Framebuffer PUBLIC Vulkan::Vulkan)

target_include_directories(Framebuffer PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Framebuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
