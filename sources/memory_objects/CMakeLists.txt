add_library(Buffers buffers.cpp)

target_link_libraries(Buffers PUBLIC Vulkan::Vulkan)
target_link_libraries(Buffers PUBLIC MemoryAllocator)

add_library(StagignBuffer staging_buffer.cpp)

target_link_libraries(StagignBuffer PUBLIC Vulkan::Vulkan)
target_link_libraries(StagignBuffer PUBLIC LogicalDevice MemoryAllocator)

add_subdirectory(texture)
add_subdirectory(uniform_buffer)

add_library(VertexBuffer vertex_buffer.cpp buffer.cpp)

target_link_libraries(VertexBuffer PUBLIC Vulkan::Vulkan)
target_link_libraries(VertexBuffer PUBLIC LogicalDevice CommandBuffer StagignBuffer)

target_include_directories(VertexBuffer PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(VertexBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(IndexBuffer index_buffer.cpp)

target_link_libraries(IndexBuffer PUBLIC Vulkan::Vulkan)
target_link_libraries(IndexBuffer PUBLIC LogicalDevice CommandBuffer StagignBuffer)

target_include_directories(IndexBuffer PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(IndexBuffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
