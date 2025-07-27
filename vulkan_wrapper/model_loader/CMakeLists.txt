add_subdirectory(obj_loader)
add_subdirectory(tiny_gltf_loader)
add_subdirectory(image_loader)

add_library(ModelLoader INTERFACE model_loader.h)
target_link_libraries(ModelLoader INTERFACE TinyGLTFLoader OBJLoader ImageLoader)

target_include_directories(ModelLoader INTERFACE ${PROJECT_SOURCE_DIR})
target_include_directories(ModelLoader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})