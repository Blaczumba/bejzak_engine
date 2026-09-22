add_library(VulkanWrapperUtil vertex_input_description_builder.h vertex_input_description_builder.cpp
util.h util.cpp
pipeline_stage_helper.h pipeline_stage_helper.cpp)

target_link_libraries(VulkanWrapperUtil PUBLIC Vulkan::Vulkan glm::glm)

target_include_directories(VulkanWrapperUtil PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(VulkanWrapperUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})