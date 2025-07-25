add_library(Buffer buffer.cpp texture.cpp buffers.cpp)
target_link_libraries(Buffer PUBLIC Vulkan::Vulkan)
target_link_libraries(Buffer PUBLIC LogicalDevice CommandBuffer)

target_include_directories(Buffer PUBLIC ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(Buffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
