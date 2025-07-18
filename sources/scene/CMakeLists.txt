add_library(Scene octree.cpp)

target_link_libraries(Scene Object)

target_include_directories(Scene PUBLIC ${CMAKE_SOURCE_DIR}/sources)
target_include_directories(Scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
