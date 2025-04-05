add_library(Instance instance.cpp)

target_link_libraries(Instance PUBLIC Vulkan::Vulkan)

target_include_directories(Instance PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Instance PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
