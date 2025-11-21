add_library(Pipeline pipeline.cpp graphics_pipeline.cpp compute_pipeline.cpp shader.cpp shader_program.cpp)

target_link_libraries(Pipeline PUBLIC Vulkan::Vulkan)
target_link_libraries(Pipeline PUBLIC LogicalDevice Renderpass)

target_include_directories(Pipeline PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Pipeline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# TODO: refactor
if (ANDROID)
    target_compile_definitions(Pipeline PUBLIC SHADERS_PATH="shaders/")
else ()
    target_compile_definitions(Pipeline PUBLIC SHADERS_PATH="${PROJECT_SOURCE_DIR}/shaders/")
endif ()