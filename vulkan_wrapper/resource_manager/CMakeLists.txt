add_library(AssetManager asset_manager.h asset_manager.cpp)

target_link_libraries(AssetManager PUBLIC Vulkan::Vulkan)
target_link_libraries(AssetManager PUBLIC CommonModelLoader LogicalDevice Buffer CommonUtil)

target_include_directories(AssetManager PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(AssetManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})