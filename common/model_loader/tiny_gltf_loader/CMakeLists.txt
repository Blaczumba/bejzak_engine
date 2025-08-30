add_library(CommonTinyGltfLoader tiny_gltf_loader.h tiny_gltf_loader.cpp)

target_include_directories(CommonTinyGltfLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonTinyGltfLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(CommonTinyGltfLoader PUBLIC ${PROJECT_SOURCE_DIR}/external)
