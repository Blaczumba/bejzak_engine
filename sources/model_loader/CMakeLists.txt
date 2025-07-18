add_library(ModelLoader model_loader.cpp)

add_subdirectory(obj_loader)
add_subdirectory(tiny_gltf_loader)
add_subdirectory(image_loader)

target_include_directories(ModelLoader PUBLIC ${PROJECT_SOURCE_DIR}/sources)
target_include_directories(ModelLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})