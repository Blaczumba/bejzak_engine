add_subdirectory(attachment)

add_library(Renderpass render_pass.cpp)

target_link_libraries(Renderpass PUBLIC LogicalDevice Attachment)
target_link_libraries(Renderpass PUBLIC Vulkan::Vulkan)

target_include_directories(Renderpass PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Renderpass PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
