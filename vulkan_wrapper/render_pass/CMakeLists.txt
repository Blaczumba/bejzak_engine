add_library(Renderpass render_pass.cpp attachment_layout.cpp)

target_link_libraries(Renderpass PUBLIC LogicalDevice)
target_link_libraries(Renderpass PUBLIC Vulkan::Vulkan)

target_include_directories(Renderpass PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Renderpass PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
