add_library(Buffer
	buffer.h buffer.cpp
	texture.h texture.cpp
	buffers.cpp buffer_deallocator.h image.h
)
target_link_libraries(Buffer PUBLIC Vulkan::Vulkan)
target_link_libraries(Buffer PUBLIC LogicalDevice CommandBuffer)

target_include_directories(Buffer PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Buffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
