add_library(TinyGLTFLoader tiny_gltf_loader.cpp)

target_link_libraries(TinyGLTFLoader PUBLIC ModelLoader)

target_include_directories(TinyGLTFLoader INTERFACE ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(TinyGLTFLoader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(TinyGLTFLoader INTERFACE ${CMAKE_SOURCE_DIR}/external)
