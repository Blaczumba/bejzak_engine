add_library(Instance instance.h instance.cpp validation_layers.h)

target_link_libraries(Instance PUBLIC Vulkan::Vulkan)

target_include_directories(Instance PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Instance PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
