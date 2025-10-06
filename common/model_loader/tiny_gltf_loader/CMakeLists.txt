add_library(CommonTinyGltfLoader tiny_gltf_loader.h tiny_gltf_loader.cpp)

target_link_libraries(CommonTinyGltfLoader PRIVATE CommonUtil)

target_compile_definitions(CommonTinyGltfLoader PUBLIC TINYGLTF_NO_EXTERNAL_IMAGE TINYGLTF_NO_STB_IMAGE_WRITE)

if (ANDROID)
    target_compile_definitions(CommonTinyGltfLoader PRIVATE TINYGLTF_ANDROID_LOAD_FROM_ASSETS)
endif ()

target_include_directories(CommonTinyGltfLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonTinyGltfLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(CommonTinyGltfLoader PUBLIC ${PROJECT_SOURCE_DIR}/external)
