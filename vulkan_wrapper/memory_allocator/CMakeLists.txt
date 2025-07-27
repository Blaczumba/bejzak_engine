add_library(MemoryAllocator memory_allocator.h memory_allocator.cpp allocation.h)

target_include_directories(MemoryAllocator PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(MemoryAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
