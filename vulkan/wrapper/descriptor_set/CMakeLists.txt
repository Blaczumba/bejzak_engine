add_library(DescriptorSet 
	descriptor_set.h descriptor_set.cpp
	descriptor_pool.h descriptor_pool.cpp
	descriptor_set_layout.h descriptor_set_layout.cpp
	descriptor_set_writer.h descriptor_set_writer.cpp
	descriptor_set_writer_lib.h
    lib.h lib.cpp)

target_link_libraries(DescriptorSet PRIVATE Vulkan::Vulkan)
target_link_libraries(DescriptorSet PUBLIC LogicalDevice MemoryObjects)

target_include_directories(DescriptorSet PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(DescriptorSet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})