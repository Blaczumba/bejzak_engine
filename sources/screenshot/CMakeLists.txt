add_library(Screenshot screenshot.cpp)

target_link_libraries(Screenshot PUBLIC Vulkan::Vulkan)
target_link_libraries(Screenshot PUBLIC CommandBuffer Swapchain Buffers)

target_include_directories(Screenshot PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Screenshot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})