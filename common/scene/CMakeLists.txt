add_library(CommonScene octree.h octree.cpp)

target_link_libraries(CommonScene CommonObject CommonPrimitives)

target_include_directories(CommonScene PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(CommonScene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
