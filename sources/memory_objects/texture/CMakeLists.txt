add_library(Texture texture.cpp)

target_link_libraries(Texture PUBLIC Vulkan::Vulkan)
target_link_libraries(Texture PUBLIC LogicalDevice Buffers StagignBuffer ImageLoader)

target_include_directories(Texture PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Texture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})