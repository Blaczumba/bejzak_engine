add_library(OBJLoader obj_loader.cpp)

target_link_libraries(OBJLoader PRIVATE tinyobjloader)
target_link_libraries(OBJLoader PUBLIC ModelLoader)

target_include_directories(OBJLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(OBJLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})