add_library(Shader shader.cpp shader_program.cpp)

target_link_libraries(Shader PUBLIC Vulkan::Vulkan)
target_link_libraries(Shader PUBLIC LogicalDevice DescriptorSet)

target_include_directories(Shader PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Shader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(Shader PUBLIC SHADERS_PATH="${CMAKE_SOURCE_DIR}/shaders/")