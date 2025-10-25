add_library(MemoryAllocator memory_allocator.h memory_allocator.cpp allocation.h
        ${PROJECT_SOURCE_DIR}/external/vma/vk_mem_alloc.h)

target_link_libraries(MemoryAllocator PUBLIC Vulkan::Vulkan)

target_include_directories(MemoryAllocator PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(MemoryAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
