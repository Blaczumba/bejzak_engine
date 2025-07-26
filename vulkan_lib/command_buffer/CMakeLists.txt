add_library(CommandBuffer command_buffer.cpp)

target_link_libraries(CommandBuffer PUBLIC LogicalDevice Framebuffer)

target_include_directories(CommandBuffer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommandBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})