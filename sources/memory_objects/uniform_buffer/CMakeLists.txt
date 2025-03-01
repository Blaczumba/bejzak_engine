add_library(UniformBuffer INTERFACE uniform_buffer.h push_constants.h)

target_link_libraries(UniformBuffer INTERFACE Vulkan::Vulkan)
target_link_libraries(UniformBuffer INTERFACE LogicalDevice Texture Buffers)

target_include_directories(UniformBuffer INTERFACE ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(UniformBuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})