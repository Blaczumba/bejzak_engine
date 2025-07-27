add_library(TinyGLTFLoader tiny_gltf_loader.h tiny_gltf_loader.cpp)

target_link_libraries(TinyGLTFLoader PUBLIC ModelLoader)

target_include_directories(TinyGLTFLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(TinyGLTFLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(TinyGLTFLoader PUBLIC ${PROJECT_SOURCE_DIR}/external)
