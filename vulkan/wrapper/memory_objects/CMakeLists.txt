add_library(MemoryObjects
	buffer.h buffer.cpp
	image.h image.cpp
	memory_objects_lib.h memory_objects_lib.cpp)
target_link_libraries(MemoryObjects PUBLIC Vulkan::Vulkan)
target_link_libraries(MemoryObjects PUBLIC LogicalDevice CommandBuffer)

target_include_directories(MemoryObjects PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(MemoryObjects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
