add_library(Framebuffer framebuffer.h framebuffer.cpp)

target_link_libraries(Framebuffer PUBLIC LogicalDevice Renderpass)
target_link_libraries(Framebuffer PUBLIC Vulkan::Vulkan)

target_include_directories(Framebuffer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Framebuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
