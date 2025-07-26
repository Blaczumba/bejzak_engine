add_library(Scene octree.cpp)

target_link_libraries(Scene Object)

target_include_directories(Scene PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(Scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
