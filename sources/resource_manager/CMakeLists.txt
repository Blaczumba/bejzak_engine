add_library(AssetManager asset_manager.cpp "resource_manager.h" "resource_manager.cpp")

target_link_libraries(AssetManager PUBLIC Vulkan::Vulkan)
target_link_libraries(AssetManager PUBLIC ImageLoader LogicalDevice VertexBuffer IndexBuffer StagignBuffer ThreadPool Primitives)

target_include_directories(AssetManager PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(AssetManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})