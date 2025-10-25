add_library(CommonObjLoader obj_loader.h obj_loader.cpp)

target_link_libraries(CommonObjLoader PRIVATE tinyobjloader)

target_include_directories(CommonObjLoader PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonObjLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})