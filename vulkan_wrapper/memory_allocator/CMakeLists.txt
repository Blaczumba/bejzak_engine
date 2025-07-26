add_library(MemoryAllocator memory_allocator.cpp memory_allocator.h allocation.h)

# target_link_libraries(MemoryAllocator PUBLIC Texture)

target_include_directories(MemoryAllocator PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(MemoryAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
